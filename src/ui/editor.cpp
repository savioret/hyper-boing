#include "editor.h"
#include "../main.h"
#include "../core/coordhelper.h"
#include "../core/logger.h"
#include "../game/stageloader.h"
#include "../game/scene.h"
#include <cmath>
#include <algorithm>

// ============================================================
// EditorObject
// ============================================================

CollisionBox EditorObject::getHitBox() const
{
    // For bottom-middle types (Pickup, Ladder), convert to top-left
    if (type == StageObjectType::Item || type == StageObjectType::Ladder)
    {
        int rx = toRenderX(x, spriteW);
        int ry = toRenderY(y, spriteH);
        return { rx, ry, spriteW, spriteH };
    }
    // Top-left types (Ball, Floor, Glass, Hexa)
    return { (int)x, (int)y, spriteW, spriteH };
}

static int snapToGrid(int v, bool snap)
{
    if (!snap) return v;
    const int G = 8;
    return (v + G / 2) / G * G;
}

static std::unique_ptr<EditorObject> cloneEditorObject(const EditorObject& src)
{
    auto eo = std::make_unique<EditorObject>();
    eo->type     = src.type;
    eo->x        = src.x;
    eo->y        = src.y;
    eo->startTime = src.startTime;
    eo->spriteW  = src.spriteW;
    eo->spriteH  = src.spriteH;
    eo->selected = false;
    if (src.params)
        eo->params = src.params->clone();
    return eo;
}

// ============================================================
// Editor - Construction / Lifecycle
// ============================================================

Editor::Editor(Stage* stg)
    : stage(stg)
{
    playerX[0] = (float)stg->xpos[0];
    playerY[0] = (float)stg->ypos[0];
    playerX[1] = (float)stg->xpos[1];
    playerY[1] = (float)stg->ypos[1];
}

Editor::~Editor()
{
}

int Editor::init()
{
    GameState::init();

    // Load background
    char path[260];
    std::snprintf(path, sizeof(path), "assets/graph/bg/%s", stage->back);
    backgroundSprite.init(&appGraph, path, 16, 16);
    appGraph.setColorKey(backgroundSprite.getBmp(), 0x00FF00);

    // Populate editor objects from stage sequence
    loadFromStage();

    setStatus("Editor mode - Ctrl+E to return to game");
    LOG_INFO("Editor: loaded %d objects from stage %d", (int)objects.size(), stage->id);

    return 1;
}

GameState* Editor::moveAll(float dt)
{
    // Update floating object position
    if (floatingObj)
    {
        if (isBottomMiddleType(floatingObj->type))
        {
            floatingObj->x = (float)snapToGrid(mouseX, showGrid);
            floatingObj->y = (float)snapToGrid(mouseY, showGrid);
        }
        else
        {
            floatingObj->x = (float)snapToGrid(mouseX - floatingObj->spriteW / 2, showGrid);
            floatingObj->y = (float)snapToGrid(mouseY - floatingObj->spriteH / 2, showGrid);
        }
    }

    // Fade status message
    if (statusTimer > 0.0f)
    {
        statusTimer -= dt;
        if (statusTimer <= 0.0f)
            statusMsg.clear();
    }

    return nullptr;
}

int Editor::drawAll()
{
    drawBackground();
    Scene::drawMark(appGraph, gameinf.getStageRes());
    drawPlayfieldBorder();
    drawObjects();
    drawPlayers();
    drawFloatingObject();
    drawStatusBar();

    if (showingPickupModal)
        drawPickupModal();

    finalizeRender();
    return 1;
}

int Editor::release()
{
    backgroundSprite.release();
    objects.clear();
    floatingObj.reset();
    return 1;
}

// ============================================================
// SDL Event Handling
// ============================================================

void Editor::onSDLEvent(const SDL_Event& e)
{
    switch (e.type)
    {
    case SDL_MOUSEBUTTONDOWN:
    {
        int lx, ly;
        windowToLogical(e.button.x, e.button.y, lx, ly);
        onMouseDown(lx, ly, e.button.button);
        break;
    }
    case SDL_MOUSEBUTTONUP:
    {
        int lx, ly;
        windowToLogical(e.button.x, e.button.y, lx, ly);
        onMouseUp(lx, ly, e.button.button);
        break;
    }
    case SDL_MOUSEMOTION:
    {
        int lx, ly;
        windowToLogical(e.motion.x, e.motion.y, lx, ly);
        mouseX = lx;
        mouseY = ly;
        onMouseMove(lx, ly);
        break;
    }
    case SDL_MOUSEWHEEL:
        onMouseWheel(e.wheel.y);
        break;
    case SDL_KEYDOWN:
        onKeyDown(e.key.keysym.sym, e.key.keysym.mod);
        break;
    }
}

void Editor::windowToLogical(int winX, int winY, int& outX, int& outY)
{
    // SDL_RenderSetLogicalSize already automatically scales mouse coordinates
    // from window space to logical space. Therefore, winX and winY from SDL_Event
    // are already in the 0..RES_X, 0..RES_Y space. We only need to clamp them.
    outX = std::max(0, std::min(RES_X - 1, winX));
    outY = std::max(0, std::min(RES_Y - 1, winY));
}

// ============================================================
// Input Handlers
// ============================================================

void Editor::onMouseDown(int mx, int my, int button)
{
    if (button == SDL_BUTTON_LEFT)
    {
        mouseDown = true;

        // If floating object, place it
        if (floatingObj)
        {
            placeFloatingObject();
            return;
        }

        // Check player hit first
        for (int i = 0; i < 2; i++)
        {
            CollisionBox pb = getPlayerHitBox(i);
            if (contains(pb, mx, my))
            {
                draggedPlayer = i;
                dragOffsetX = mx - (int)playerX[i];
                dragOffsetY = my - (int)playerY[i];
                deselectAll();
                return;
            }
        }

        // Check object hit
        EditorObject* hit = findObjectAt(mx, my);
        if (hit)
        {
            selectObject(hit->id);
            isDragging = true;
            CollisionBox hb = hit->getHitBox();
            dragOffsetX = mx - hb.x;
            dragOffsetY = my - hb.y;
        }
        else
        {
            deselectAll();
        }
    }
    else if (button == SDL_BUTTON_RIGHT)
    {
        // Cycle color
        if (floatingObj)
        {
            cycleColor(*floatingObj, 1);
        }
        else if (EditorObject* sel = findById(selectedId))
        {
            cycleColor(*sel, 1);
            dirty = true;
        }
    }
}

void Editor::onMouseUp(int mx, int my, int button)
{
    if (button == SDL_BUTTON_LEFT)
    {
        mouseDown = false;
        isDragging = false;
        draggedPlayer = -1;
    }
}

void Editor::onMouseMove(int mx, int my)
{
    if (draggedPlayer >= 0)
    {
        playerX[draggedPlayer] = (float)snapToGrid(mx - dragOffsetX, showGrid);
        playerY[draggedPlayer] = (float)snapToGrid(my - dragOffsetY, showGrid);
        dirty = true;
        return;
    }

    if (isDragging)
    {
        EditorObject* sel = findById(selectedId);
        if (sel)
        {
            if (isBottomMiddleType(sel->type))
            {
                // hitbox top-left = (toRenderX(x,w), toRenderY(y,h))
                // Reverse: x = hitbox_x + w/2,  y = hitbox_y + h
                sel->x = (float)snapToGrid(mx - dragOffsetX + sel->spriteW / 2, showGrid);
                sel->y = (float)snapToGrid(my - dragOffsetY + sel->spriteH, showGrid);
            }
            else
            {
                sel->x = (float)snapToGrid(mx - dragOffsetX, showGrid);
                sel->y = (float)snapToGrid(my - dragOffsetY, showGrid);
            }
            dirty = true;
        }
    }
}

void Editor::onMouseWheel(int direction)
{
    if (floatingObj)
    {
        cycleSizeOrType(*floatingObj, direction);
    }
    else if (EditorObject* sel = findById(selectedId))
    {
        cycleSizeOrType(*sel, direction);
        dirty = true;
    }
}

void Editor::onKeyDown(SDL_Keycode key, Uint16 mod)
{
    // Ctrl+E is handled by GameRunner, not here

    // Pickup modal intercepts all keys while open
    if (showingPickupModal)
    {
        switch (key)
        {
        case SDLK_LEFT:  pickupModalType = (pickupModalType + 6) % 7; break;
        case SDLK_RIGHT: pickupModalType = (pickupModalType + 1) % 7; break;
        case SDLK_UP:    pickupModalSize = std::max(0, pickupModalSize - 1); break;
        case SDLK_DOWN:  pickupModalSize++; break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: confirmPickupModal(); break;
        case SDLK_DELETE:
        case SDLK_BACKSPACE: removeLastPickup(); break;
        case SDLK_ESCAPE: showingPickupModal = false; break;
        default: break;
        }
        return;
    }

    switch (key)
    {
    // Number keys 1-6: spawn floating object
    case SDLK_1: startPlacing(StageObjectType::Ball);   break;
    case SDLK_2: startPlacing(StageObjectType::Floor);  break;
    case SDLK_3: startPlacing(StageObjectType::Glass);  break;
    case SDLK_4: startPlacing(StageObjectType::Ladder); break;
    case SDLK_5: startPlacing(StageObjectType::Item);   break;
    case SDLK_6: startPlacing(StageObjectType::Hexa);   break;

    case SDLK_q:
        if (floatingObj)
            cycleSizeOrType(*floatingObj, 1);
        else if (EditorObject* sel = findById(selectedId))
        {
            cycleSizeOrType(*sel, 1);
            dirty = true;
        }
        break;

    case SDLK_w:
        if (floatingObj)
            cycleColor(*floatingObj, 1);
        else if (EditorObject* sel = findById(selectedId))
        {
            cycleColor(*sel, 1);
            dirty = true;
        }
        break;

    case SDLK_DELETE:
        deleteSelected();
        break;

    case SDLK_ESCAPE:
        // Cancel floating object insertion; dialog is handled by GameState base class
        if (floatingObj)
            floatingObj.reset();
        break;

    case SDLK_b:
        showBBoxes = !showBBoxes;
        setStatus(showBBoxes ? "Bounding boxes ON" : "Bounding boxes OFF");
        break;

    case SDLK_g:
        showGrid = !showGrid;
        setStatus(showGrid ? "Grid ON" : "Grid OFF");
        break;

    case SDLK_i:
        if (EditorObject* sel = findById(selectedId))
        {
            if (sel->type == StageObjectType::Floor)
            {
                if (auto* p = dynamic_cast<FloorParams*>(sel->params.get()))
                {
                    p->invisible = !p->invisible;
                    setStatus(p->invisible ? "Invisible ON" : "Invisible OFF");
                    dirty = true;
                }
            }
            else if (sel->type == StageObjectType::Glass)
            {
                if (auto* p = dynamic_cast<GlassParams*>(sel->params.get()))
                {
                    p->invisible = !p->invisible;
                    setStatus(p->invisible ? "Invisible ON" : "Invisible OFF");
                    dirty = true;
                }
            }
        }
        break;

    case SDLK_p:
        if (EditorObject* sel = findById(selectedId))
        {
            if (sel->type == StageObjectType::Floor)
            {
                if (auto* p = dynamic_cast<FloorParams*>(sel->params.get()))
                {
                    p->passthrough = !p->passthrough;
                    setStatus(p->passthrough ? "Passthrough ON" : "Passthrough OFF");
                    dirty = true;
                }
            }
            else if (sel->type == StageObjectType::Glass)
            {
                if (auto* p = dynamic_cast<GlassParams*>(sel->params.get()))
                {
                    p->passthrough = !p->passthrough;
                    setStatus(p->passthrough ? "Passthrough ON" : "Passthrough OFF");
                    dirty = true;
                }
            }
        }
        break;

    case SDLK_a:
        if (findById(selectedId))
            openPickupModal();
        break;

    case SDLK_c:
    {
        EditorObject* sel = findById(selectedId);
        const EditorObject* tmpl = sel ? sel : cloneTemplate.get();
        if (!tmpl) break;

        // Save a persistent template for next C press without selection
        if (sel)
            cloneTemplate = cloneEditorObject(*sel);

        // Start placing a clone at mouse position (same logic as moveAll floating update)
        floatingObj = cloneEditorObject(*tmpl);
        floatingObj->id = 0;
        if (isBottomMiddleType(floatingObj->type))
        {
            floatingObj->x = (float)snapToGrid(mouseX, showGrid);
            floatingObj->y = (float)snapToGrid(mouseY, showGrid);
        }
        else
        {
            floatingObj->x = (float)snapToGrid(mouseX - floatingObj->spriteW / 2, showGrid);
            floatingObj->y = (float)snapToGrid(mouseY - floatingObj->spriteH / 2, showGrid);
        }
        setStatus("Clone: click to place, Esc to cancel");
        break;
    }

    case SDLK_F1:
        saveToFile();
        break;

    // Ctrl+Arrows: adjust velocity/direction
    case SDLK_LEFT:
    case SDLK_RIGHT:
    case SDLK_UP:
    case SDLK_DOWN:
        if (mod & KMOD_CTRL)
        {
            EditorObject* sel = findById(selectedId);
            if (sel && sel->type == StageObjectType::Ball)
            {
                if (auto* p = dynamic_cast<BallParams*>(sel->params.get()))
                {
                    if (key == SDLK_LEFT)  p->dirX = std::max(-10.0f, p->dirX - 0.2f);
                    if (key == SDLK_RIGHT) p->dirX = std::min( 10.0f, p->dirX + 0.2f);
                    if (key == SDLK_UP)    p->dirY = -1;
                    if (key == SDLK_DOWN)  p->dirY = 1;
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "Ball: dirX=%.1f dirY=%d", p->dirX, p->dirY);
                    setStatus(buf);
                    dirty = true;
                }
            }
            else if (sel && sel->type == StageObjectType::Hexa)
            {
                if (auto* p = dynamic_cast<HexaParams*>(sel->params.get()))
                {
                    if (key == SDLK_LEFT)  p->velX = std::max(-10.0f, p->velX - 0.2f);
                    if (key == SDLK_RIGHT) p->velX = std::min( 10.0f, p->velX + 0.2f);
                    if (key == SDLK_UP)    p->velY = std::max(-10.0f, p->velY - 0.2f);
                    if (key == SDLK_DOWN)  p->velY = std::min( 10.0f, p->velY + 0.2f);
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "Hexa: velX=%.1f velY=%.1f", p->velX, p->velY);
                    setStatus(buf);
                    dirty = true;
                }
            }
        }
        break;
    }
}

// ============================================================
// Object Operations
// ============================================================

EditorObject* Editor::findObjectAt(int mx, int my)
{
    // Search in reverse order (top-most drawn last = first hit)
    for (int i = (int)objects.size() - 1; i >= 0; i--)
    {
        CollisionBox hb = objects[i].getHitBox();
        if (contains(hb, mx, my))
            return &objects[i];
    }
    return nullptr;
}

EditorObject* Editor::findById(int id)
{
    if (id < 0) return nullptr;
    for (auto& obj : objects)
    {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

void Editor::selectObject(int id)
{
    deselectAll();
    selectedId = id;
    if (EditorObject* obj = findById(id))
        obj->selected = true;
}

void Editor::deselectAll()
{
    selectedId = -1;
    for (auto& obj : objects)
        obj.selected = false;
}

void Editor::deleteSelected()
{
    if (selectedId < 0) return;
    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [this](const EditorObject& o) { return o.id == selectedId; }),
        objects.end());
    selectedId = -1;
    dirty = true;
    setStatus("Object deleted");
}

void Editor::startPlacing(StageObjectType type)
{
    floatingObj = std::make_unique<EditorObject>();
    floatingObj->id = -1;  // Not yet placed
    floatingObj->type = type;
    floatingObj->startTime = 0;

    switch (type)
    {
    case StageObjectType::Ball:
        floatingObj->params = std::make_unique<BallParams>();
        break;
    case StageObjectType::Floor:
        floatingObj->params = std::make_unique<FloorParams>();
        break;
    case StageObjectType::Glass:
        floatingObj->params = std::make_unique<GlassParams>();
        break;
    case StageObjectType::Ladder:
        floatingObj->params = std::make_unique<LadderParams>();
        break;
    case StageObjectType::Item:
        floatingObj->params = std::make_unique<PickupParams>();
        break;
    case StageObjectType::Hexa:
        floatingObj->params = std::make_unique<HexaParams>();
        break;
    default:
        floatingObj.reset();
        return;
    }

    updateCachedDimensions(*floatingObj);
    floatingObj->x = (float)mouseX;
    floatingObj->y = (float)mouseY;

    const char* names[] = { "", "Ball", "", "Floor", "Pickup", "", "", "Ladder", "Glass", "Hexa" };
    int idx = static_cast<int>(type);
    if (idx >= 0 && idx <= 9)
        setStatus(std::string("Placing ") + names[idx] + " - click to place, Esc to cancel");
}

void Editor::placeFloatingObject()
{
    if (!floatingObj) return;

    floatingObj->id = nextId++;
    floatingObj->selected = false;
    objects.push_back(std::move(*floatingObj));
    floatingObj.reset();
    dirty = true;
    setStatus("Object placed");
}

// ============================================================
// Property Cycling
// ============================================================

void Editor::cycleSizeOrType(EditorObject& obj, int dir)
{
    switch (obj.type)
    {
    case StageObjectType::Ball:
        if (auto* p = dynamic_cast<BallParams*>(obj.params.get()))
        {
            p->size = (p->size + dir + 4) % 4;
            char buf[32];
            std::snprintf(buf, sizeof(buf), "Ball size: %d", p->size);
            setStatus(buf);
        }
        break;

    case StageObjectType::Floor:
        if (auto* p = dynamic_cast<FloorParams*>(obj.params.get()))
        {
            int v = static_cast<int>(p->floorType);
            v = (v + dir + 5) % 5;
            p->floorType = static_cast<FloorType>(v);
        }
        break;

    case StageObjectType::Glass:
        if (auto* p = dynamic_cast<GlassParams*>(obj.params.get()))
        {
            // GlassType values: 0, 5, 10, 15, 20
            static const int glassVals[] = { 0, 5, 10, 15, 20 };
            int cur = 0;
            for (int i = 0; i < 5; i++)
                if (glassVals[i] == static_cast<int>(p->glassType)) { cur = i; break; }
            cur = (cur + dir + 5) % 5;
            p->glassType = static_cast<GlassType>(glassVals[cur]);
        }
        break;

    case StageObjectType::Hexa:
        if (auto* p = dynamic_cast<HexaParams*>(obj.params.get()))
        {
            p->size = (p->size + dir + 3) % 3;
            char buf[32];
            std::snprintf(buf, sizeof(buf), "Hexa size: %d", p->size);
            setStatus(buf);
        }
        break;

    case StageObjectType::Item:
        if (auto* p = dynamic_cast<PickupParams*>(obj.params.get()))
        {
            int v = static_cast<int>(p->pickupType);
            v = (v + dir + 7) % 7;
            p->pickupType = static_cast<PickupType>(v);
        }
        break;

    case StageObjectType::Ladder:
        if (auto* p = dynamic_cast<LadderParams*>(obj.params.get()))
        {
            p->numTiles = std::max(1, std::min(15, p->numTiles + dir));
        }
        break;

    default:
        break;
    }
    updateCachedDimensions(obj);
}

void Editor::cycleColor(EditorObject& obj, int dir)
{
    switch (obj.type)
    {
    case StageObjectType::Ball:
        if (auto* p = dynamic_cast<BallParams*>(obj.params.get()))
            p->ballType = (p->ballType + dir + 3) % 3;
        break;

    case StageObjectType::Floor:
        if (auto* p = dynamic_cast<FloorParams*>(obj.params.get()))
            p->floorColor = (p->floorColor + dir + 4) % 4;
        break;

    case StageObjectType::Glass:
        if (auto* p = dynamic_cast<GlassParams*>(obj.params.get()))
            p->glassColor = (p->glassColor + dir + 4) % 4;
        break;

    case StageObjectType::Hexa:
        if (auto* p = dynamic_cast<HexaParams*>(obj.params.get()))
            p->hexaColor = (p->hexaColor + dir + 4) % 4;
        break;

    default:
        break;
    }
    updateCachedDimensions(obj);
}

// ============================================================
// Sprite Lookup
// ============================================================

Sprite* Editor::getSpriteForObject(const EditorObject& obj) const
{
    StageResources& res = AppData::instance().getStageRes();

    switch (obj.type)
    {
    case StageObjectType::Ball:
    {
        auto* p = dynamic_cast<const BallParams*>(obj.params.get());
        int color = p ? p->ballType : 0;
        int size = p ? p->size : 0;
        AnimSpriteSheet* anim = res.getBallAnim(color);
        return anim ? anim->getFrame(size) : nullptr;
    }
    case StageObjectType::Hexa:
    {
        auto* p = dynamic_cast<const HexaParams*>(obj.params.get());
        int color = p ? p->hexaColor : 0;
        int size = p ? p->size : 0;
        // Hexa frames: size 0 = frames 0-3, size 1 = frames 4-7, size 2 = frames 8-11
        AnimSpriteSheet* anim = res.getHexaAnim(color);
        return anim ? anim->getFrame(size * 4) : nullptr;
    }
    case StageObjectType::Floor:
    {
        auto* p = dynamic_cast<const FloorParams*>(obj.params.get());
        int color = p ? p->floorColor : 0;
        int typeIdx = static_cast<int>(p ? p->floorType : FloorType::HORIZ_BIG);
        AnimSpriteSheet* anim = res.getFloorAnim(color);
        return anim ? anim->getFrame(typeIdx) : nullptr;
    }
    case StageObjectType::Glass:
    {
        auto* p = dynamic_cast<const GlassParams*>(obj.params.get());
        int color = p ? p->glassColor : 0;
        int typeIdx = static_cast<int>(p ? p->glassType : GlassType::HORIZ_BIG);
        AnimSpriteSheet* anim = res.getGlassAnim(color);
        return anim ? anim->getFrame(typeIdx) : nullptr;
    }
    case StageObjectType::Item:
    {
        auto* p = dynamic_cast<const PickupParams*>(obj.params.get());
        int idx = static_cast<int>(p ? p->pickupType : PickupType::GUN);
        if (idx == static_cast<int>(PickupType::SHIELD))
        {
            return res.pickupShieldAnim ? res.pickupShieldAnim->getFrame(0) : nullptr;
        }
        // CLAW (enum 6) maps to array index 5
        if (idx == static_cast<int>(PickupType::CLAW))
            idx = 5;
        if (idx >= 0 && idx < 6)
            return &res.pickupSprites[idx];
        return nullptr;
    }
    case StageObjectType::Ladder:
    {
        // Return the single tile sprite; draw code handles tiling
        return &res.ladder;
    }
    default:
        return nullptr;
    }
}

void Editor::updateCachedDimensions(EditorObject& obj)
{
    if (obj.type == StageObjectType::Ladder)
    {
        auto* p = dynamic_cast<const LadderParams*>(obj.params.get());
        Sprite* spr = getSpriteForObject(obj);
        if (spr && p)
        {
            obj.spriteW = spr->getWidth();
            obj.spriteH = spr->getHeight() * p->numTiles;
        }
        return;
    }

    Sprite* spr = getSpriteForObject(obj);
    if (spr)
    {
        obj.spriteW = spr->getWidth();
        obj.spriteH = spr->getHeight();
    }
    else
    {
        obj.spriteW = 16;
        obj.spriteH = 16;
    }
}

// ============================================================
// Drawing
// ============================================================

void Editor::drawBackground()
{
    appGraph.draw(&backgroundSprite, 0, 0);
}

void Editor::drawPlayfieldBorder()
{
    // Draw 8px snap grid at 50% alpha
    if (showGrid)
    {
        SDL_SetRenderDrawBlendMode(appGraph.getRenderer(), SDL_BLENDMODE_BLEND);
        appGraph.setDrawColor(160, 160, 160, 89);   // 35% alpha
        for (int gx = Stage::MIN_X; gx <= Stage::MAX_X; gx += 8)
            SDL_RenderDrawLine(appGraph.getRenderer(), gx, Stage::MIN_Y, gx, Stage::MAX_Y);
        for (int gy = Stage::MIN_Y; gy <= Stage::MAX_Y; gy += 8)
            SDL_RenderDrawLine(appGraph.getRenderer(), Stage::MIN_X, gy, Stage::MAX_X, gy);
        SDL_SetRenderDrawBlendMode(appGraph.getRenderer(), SDL_BLENDMODE_NONE);
    }

}

void Editor::drawObjects()
{
    for (const auto& obj : objects)
    {
        drawObject(obj);
        if (showBBoxes)
            drawBoundingBox(obj);
        bool needArrow = (showBBoxes || obj.selected) &&
                         (obj.type == StageObjectType::Ball || obj.type == StageObjectType::Hexa);
        if (needArrow)
            drawVelocityArrow(obj);
        if (showBBoxes || obj.selected)
            drawDeathPickupIcons(obj);
        if (obj.selected)
            drawSelectionHighlight(obj);
    }
}

void Editor::drawObject(const EditorObject& obj)
{
    Sprite* spr = getSpriteForObject(obj);
    if (!spr) return;

    if (obj.type == StageObjectType::Ladder)
    {
        // Draw tiled ladder
        auto* p = dynamic_cast<const LadderParams*>(obj.params.get());
        int numTiles = p ? p->numTiles : 3;
        int drawX = toRenderX(obj.x, spr->getWidth());
        int topY = toRenderY(obj.y, spr->getHeight() * numTiles);
        for (int i = 0; i < numTiles; i++)
            appGraph.draw(spr, drawX, topY + i * spr->getHeight());
        return;
    }

    if (isBottomMiddleType(obj.type))
    {
        int rx = toRenderX(obj.x, spr->getWidth());
        int ry = toRenderY(obj.y, spr->getHeight());
        appGraph.draw(spr, rx, ry);
    }
    else
    {
        appGraph.draw(spr, (int)obj.x, (int)obj.y);
    }

    // Draw invisible/passthrough indicators
    if (obj.type == StageObjectType::Floor || obj.type == StageObjectType::Glass)
    {
        bool invis = false, passthru = false;
        if (auto* p = dynamic_cast<const FloorParams*>(obj.params.get()))
        {
            invis = p->invisible;
            passthru = p->passthrough;
        }
        else if (auto* p = dynamic_cast<const GlassParams*>(obj.params.get()))
        {
            invis = p->invisible;
            passthru = p->passthrough;
        }

        CollisionBox hb = obj.getHitBox();
        if (invis)
        {
            appGraph.setDrawColor(200, 200, 0, 255);
            appGraph.text("INV", hb.x, hb.y - 10);
        }
        if (passthru)
        {
            appGraph.setDrawColor(0, 200, 200, 255);
            appGraph.text("PASS", hb.x + 30, hb.y - 10);
        }
    }
}

void Editor::drawPlayers()
{
    GameBitmaps& bmp = gameinf.getBmp();
    for (int i = 0; i < 2; i++)
    {
        Sprite* spr = &bmp.player[i][5];  // ANIM_SHOOT = idle frame
        if (!spr) continue;
        int rx = toRenderX(playerX[i], spr->getWidth());
        int ry = toRenderY(playerY[i], spr->getHeight());
        appGraph.draw(spr, rx, ry);

        // Label
        appGraph.setDrawColor(255, 255, 255, 255);
        char label[4];
        std::snprintf(label, sizeof(label), "P%d", i + 1);
        appGraph.text(label, rx, ry - 10);

        // Highlight if being dragged
        if (draggedPlayer == i)
        {
            appGraph.setDrawColor(0, 255, 0, 255);
            CollisionBox pb = getPlayerHitBox(i);
            appGraph.rectangle(pb.x - 1, pb.y - 1, pb.x + pb.w + 1, pb.y + pb.h + 1);
        }
    }
}

void Editor::drawFloatingObject()
{
    if (!floatingObj) return;

    Sprite* spr = getSpriteForObject(*floatingObj);
    if (!spr) return;

    // Draw semi-transparent (use normal draw - SDL doesn't support per-draw alpha easily)
    if (floatingObj->type == StageObjectType::Ladder)
    {
        auto* p = dynamic_cast<const LadderParams*>(floatingObj->params.get());
        int numTiles = p ? p->numTiles : 3;
        int drawX = toRenderX(floatingObj->x, spr->getWidth());
        int topY = toRenderY(floatingObj->y, spr->getHeight() * numTiles);
        for (int i = 0; i < numTiles; i++)
            appGraph.draw(spr, drawX, topY + i * spr->getHeight());
    }
    else if (isBottomMiddleType(floatingObj->type))
    {
        int rx = toRenderX(floatingObj->x, spr->getWidth());
        int ry = toRenderY(floatingObj->y, spr->getHeight());
        appGraph.draw(spr, rx, ry);
    }
    else
    {
        appGraph.draw(spr, (int)floatingObj->x, (int)floatingObj->y);
    }

    // Draw crosshair at mouse position
    appGraph.setDrawColor(255, 255, 0, 255);
    appGraph.rectangle(mouseX - 3, mouseY - 1, mouseX + 3, mouseY + 1);
    appGraph.rectangle(mouseX - 1, mouseY - 3, mouseX + 1, mouseY + 3);
}

void Editor::drawSelectionHighlight(const EditorObject& obj)
{
    CollisionBox hb = obj.getHitBox();
    // Draw 3 nested rectangles for a thick, visible selection border
    appGraph.setDrawColor(255, 255, 0, 255);   // Yellow outer
    appGraph.rectangle(hb.x - 2, hb.y - 2, hb.x + hb.w + 2, hb.y + hb.h + 2);
    appGraph.setDrawColor(255, 128, 0, 255);   // Orange middle
    appGraph.rectangle(hb.x - 1, hb.y - 1, hb.x + hb.w + 1, hb.y + hb.h + 1);
    appGraph.setDrawColor(255, 255, 0, 255);   // Yellow inner
    appGraph.rectangle(hb.x, hb.y, hb.x + hb.w, hb.y + hb.h);
}

void Editor::drawBoundingBox(const EditorObject& obj)
{
    if (obj.selected) return;  // Already drawn by selection highlight
    CollisionBox hb = obj.getHitBox();
    appGraph.setDrawColor(80, 80, 80, 128);
    appGraph.rectangle(hb.x, hb.y, hb.x + hb.w, hb.y + hb.h);
}

void Editor::drawVelocityArrow(const EditorObject& obj)
{
    CollisionBox hb = obj.getHitBox();
    int cx = hb.x + hb.w / 2;
    int cy = hb.y + hb.h / 2;

    if (obj.type == StageObjectType::Ball)
    {
        auto* p = dynamic_cast<const BallParams*>(obj.params.get());
        if (!p) return;
        int endX = cx + (int)(p->dirX * 4.0f);   // max±10 → ±40px
        int endY = cy + p->dirY * 30;
        appGraph.setDrawColor(255, 255, 255, 255);
        appGraph.drawArrow(cx, cy, endX, endY);

        char buf[32];
        std::snprintf(buf, sizeof(buf), "dx=%.1f dy=%d", p->dirX, p->dirY);
        appGraph.text(buf, hb.x, hb.y + hb.h + 2);
    }
    else if (obj.type == StageObjectType::Hexa)
    {
        auto* p = dynamic_cast<const HexaParams*>(obj.params.get());
        if (!p) return;
        int endX = cx + (int)(p->velX * 4.0f);   // max±10 → ±40px
        int endY = cy + (int)(p->velY * 4.0f);
        appGraph.setDrawColor(255, 255, 255, 255);
        appGraph.drawArrow(cx, cy, endX, endY);

        char buf[32];
        std::snprintf(buf, sizeof(buf), "vx=%.1f vy=%.1f", p->velX, p->velY);
        appGraph.text(buf, hb.x, hb.y + hb.h + 2);
    }
}

void Editor::drawStatusBar()
{
    // Clear the entire editor info area below the playfield (MAX_Y+8 downward)
    const int BAR_Y = Stage::MAX_Y + 8;  // y=423
    appGraph.setDrawColor(0, 0, 0, 255);
    appGraph.filledRectangle(0, BAR_Y, RES_X, RES_Y);

    // Row 1 (y≈427): selected object info  |  status flags @ x=350  |  mouse coords
    const int ROW1 = BAR_Y + 4;
    if (EditorObject* sel = findById(selectedId))
    {
        char info[128];
        const char* typeNames[] = { "?", "Ball", "Shot", "Floor", "Pickup", "Player", "Action", "Ladder", "Glass", "Hexa" };
        int ti = static_cast<int>(sel->type);
        const char* tn = (ti >= 0 && ti <= 9) ? typeNames[ti] : "?";
        std::snprintf(info, sizeof(info), "Selected: %s at (%d,%d)",
            tn, (int)sel->x, (int)sel->y);
        appGraph.setDrawColor(255, 255, 100, 255);
        appGraph.text(info, 5, ROW1);
    }

    if (!statusMsg.empty())
    {
        appGraph.setDrawColor(255, 200, 100, 255);
        appGraph.text(statusMsg.c_str(), 350, ROW1);
    }

    char mousePos[32];
    std::snprintf(mousePos, sizeof(mousePos), "(%d,%d)", mouseX, mouseY);
    appGraph.setDrawColor(180, 180, 180, 255);
    appGraph.text(mousePos, RES_X - 70, ROW1);

    // Row 2 (y≈443): object-specific key hints  |  object count
    const int ROW2 = BAR_Y + 20;
    if (EditorObject* sel = findById(selectedId))
    {
        const char* hint = nullptr;
        switch (sel->type)
        {
        case StageObjectType::Ball:
            hint = "Ctrl+Arrows:Dir  Q:Size  W:Color  A:Pickup";
            break;
        case StageObjectType::Hexa:
            hint = "Ctrl+Arrows:Vel  Q:Size  W:Color  A:Pickup";
            break;
        case StageObjectType::Floor:
            hint = "P:Passthrough  I:Invisible  Q:Type  W:Color";
            break;
        case StageObjectType::Glass:
            hint = "P:Passthrough  I:Invisible  Q:Type  W:Color  A:Pickup";
            break;
        case StageObjectType::Ladder:
            hint = "Q:Height";
            break;
        case StageObjectType::Item:
            hint = "Q:Type";
            break;
        default:
            break;
        }
        if (hint)
        {
            appGraph.setDrawColor(100, 200, 255, 255);
            appGraph.text(hint, 5, ROW2);
        }
    }

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Objects: %d  Stage: %d%s",
        (int)objects.size(), stage->id, dirty ? " *" : "");
    appGraph.setDrawColor(200, 200, 200, 255);
    appGraph.text(buf, RES_X - 200, ROW2);

    // Row 3 (y≈459): global key hints
    const int ROW3 = BAR_Y + 36;
    appGraph.setDrawColor(120, 120, 120, 255);
    appGraph.text("1-6:Place  C:Clone  Q/W:Cycle  B:Boxes  G:Grid  Del:Delete  F1:Save  Ctrl+E:Exit", 5, ROW3);
}

// ============================================================
// Save / Load
// ============================================================

void Editor::loadFromStage()
{
    objects.clear();
    nextId = 1;

    const auto& seq = stage->getSequence();
    for (const auto& stgObj : seq)
    {
        if (stgObj->id == StageObjectType::Null) continue;
        if (stgObj->id == StageObjectType::Action) continue;  // Skip actions for now

        EditorObject eo;
        eo.id = nextId++;
        eo.type = stgObj->id;
        eo.x = (float)stgObj->x;
        eo.y = (float)stgObj->y;
        eo.startTime = stgObj->start;

        if (stgObj->params)
            eo.params = stgObj->params->clone();

        updateCachedDimensions(eo);
        objects.push_back(std::move(eo));
    }
}

void Editor::writeBackToStage()
{
    // Update player positions
    stage->xpos[0] = (int)playerX[0];
    stage->ypos[0] = (int)playerY[0];
    stage->xpos[1] = (int)playerX[1];
    stage->ypos[1] = (int)playerY[1];

    // Rebuild sequence
    std::vector<std::unique_ptr<StageObject>> newSeq;
    for (const auto& obj : objects)
    {
        auto stgObj = std::make_unique<StageObject>();
        stgObj->id = obj.type;
        stgObj->start = obj.startTime;
        stgObj->x = (int)obj.x;
        stgObj->y = (int)obj.y;
        if (obj.params)
        {
            stgObj->params = obj.params->clone();
            stgObj->params->x = stgObj->x;
            stgObj->params->y = stgObj->y;
            stgObj->params->startTime = stgObj->start;
        }
        newSeq.push_back(std::move(stgObj));
    }

    stage->replaceSequence(std::move(newSeq));
}

void Editor::saveToFile()
{
    writeBackToStage();

    std::string filename = stage->stageFile;
    if (filename.empty())
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "assets/stages/stage%d.stg", stage->id);
        filename = buf;
    }

    if (StageLoader::save(*stage, filename))
    {
        dirty = false;
        setStatus("Saved to " + filename);
        LOG_INFO("Editor: saved stage to %s", filename.c_str());
    }
    else
    {
        setStatus("SAVE FAILED: " + filename);
        LOG_ERROR("Editor: failed to save stage to %s", filename.c_str());
    }
}

// ============================================================
// Helpers
// ============================================================

void Editor::setStatus(const std::string& msg)
{
    statusMsg = msg;
    statusTimer = 4.0f;
}

bool Editor::isBottomMiddleType(StageObjectType type) const
{
    return type == StageObjectType::Item || type == StageObjectType::Ladder;
}

CollisionBox Editor::getPlayerHitBox(int idx) const
{
    Sprite* spr = &gameinf.getBmp().player[idx][5];
    if (!spr) return { 0, 0, 0, 0 };
    int rx = toRenderX(playerX[idx], spr->getWidth());
    int ry = toRenderY(playerY[idx], spr->getHeight());
    return { rx, ry, spr->getWidth(), spr->getHeight() };
}

// ============================================================
// Pickup Modal
// ============================================================

Sprite* Editor::getPickupSprite(int typeInt, StageResources& res) const
{
    if (typeInt == static_cast<int>(PickupType::SHIELD))
        return res.pickupShieldAnim ? res.pickupShieldAnim->getFrame(0) : nullptr;
    int idx = (typeInt == static_cast<int>(PickupType::CLAW)) ? 5 : typeInt;
    return (idx >= 0 && idx < 6) ? &res.pickupSprites[idx] : nullptr;
}

void Editor::openPickupModal()
{
    EditorObject* sel = findById(selectedId);
    if (!sel) return;

    pickupModalType = 0;
    pickupModalSize = 0;

    if (sel->type == StageObjectType::Ball)
    {
        auto* p = dynamic_cast<BallParams*>(sel->params.get());
        if (!p) return;
        pickupModalSize = p->size;
        if (p->deathPickupCount > 0)
        {
            pickupModalType = static_cast<int>(p->deathPickups[p->deathPickupCount - 1].type);
            pickupModalSize = p->deathPickups[p->deathPickupCount - 1].size;
        }
    }
    else if (sel->type == StageObjectType::Hexa)
    {
        auto* p = dynamic_cast<HexaParams*>(sel->params.get());
        if (!p) return;
        pickupModalSize = p->size;
        if (p->deathPickupCount > 0)
        {
            pickupModalType = static_cast<int>(p->deathPickups[p->deathPickupCount - 1].type);
            pickupModalSize = p->deathPickups[p->deathPickupCount - 1].size;
        }
    }
    else if (sel->type == StageObjectType::Glass)
    {
        auto* p = dynamic_cast<GlassParams*>(sel->params.get());
        if (!p) return;
        if (p->hasDeathPickup)
            pickupModalType = static_cast<int>(p->deathPickupType);
    }
    else return;

    showingPickupModal = true;
}

void Editor::confirmPickupModal()
{
    EditorObject* sel = findById(selectedId);
    if (!sel) { showingPickupModal = false; return; }

    PickupType pt = static_cast<PickupType>(pickupModalType);

    if (sel->type == StageObjectType::Ball)
    {
        auto* p = dynamic_cast<BallParams*>(sel->params.get());
        if (p && p->deathPickupCount < MAX_DEATH_PICKUPS)
        {
            int sz = std::min(pickupModalSize, 3);
            p->deathPickups[p->deathPickupCount++] = { sz, pt };
            dirty = true;
            setStatus("Pickup added");
        }
        else
            setStatus("Max pickups reached");
    }
    else if (sel->type == StageObjectType::Hexa)
    {
        auto* p = dynamic_cast<HexaParams*>(sel->params.get());
        if (p && p->deathPickupCount < MAX_DEATH_PICKUPS)
        {
            int sz = std::min(pickupModalSize, 2);
            p->deathPickups[p->deathPickupCount++] = { sz, pt };
            dirty = true;
            setStatus("Pickup added");
        }
        else
            setStatus("Max pickups reached");
    }
    else if (sel->type == StageObjectType::Glass)
    {
        auto* p = dynamic_cast<GlassParams*>(sel->params.get());
        if (p)
        {
            p->hasDeathPickup = true;
            p->deathPickupType = pt;
            dirty = true;
            setStatus("Pickup added");
        }
    }

    showingPickupModal = false;
}

void Editor::removeLastPickup()
{
    EditorObject* sel = findById(selectedId);
    if (!sel) { showingPickupModal = false; return; }

    bool removed = false;
    if (sel->type == StageObjectType::Ball)
    {
        auto* p = dynamic_cast<BallParams*>(sel->params.get());
        if (p && p->deathPickupCount > 0) { p->deathPickupCount--; removed = true; }
    }
    else if (sel->type == StageObjectType::Hexa)
    {
        auto* p = dynamic_cast<HexaParams*>(sel->params.get());
        if (p && p->deathPickupCount > 0) { p->deathPickupCount--; removed = true; }
    }
    else if (sel->type == StageObjectType::Glass)
    {
        auto* p = dynamic_cast<GlassParams*>(sel->params.get());
        if (p && p->hasDeathPickup) { p->hasDeathPickup = false; removed = true; }
    }

    if (removed) { dirty = true; setStatus("Pickup removed"); }
    showingPickupModal = false;
}

void Editor::drawPickupModal()
{
    Graph& g = appGraph;
    StageResources& res = AppData::instance().getStageRes();

    // Semi-transparent backdrop
    SDL_SetRenderDrawBlendMode(g.getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(g.getRenderer(), 0, 0, 0, 160);
    SDL_Rect backdrop = { 0, 0, RES_X, RES_Y };
    SDL_RenderFillRect(g.getRenderer(), &backdrop);

    // Box (wide enough for 7 icons)
    SDL_SetRenderDrawColor(g.getRenderer(), 20, 20, 40, 240);
    SDL_Rect box = { 50, 155, 540, 115 };
    SDL_RenderFillRect(g.getRenderer(), &box);
    SDL_SetRenderDrawBlendMode(g.getRenderer(), SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(g.getRenderer(), 200, 200, 220, 255);
    SDL_RenderDrawRect(g.getRenderer(), &box);

    // Title / key hints
    g.setDrawColor(255, 255, 255, 255);
    g.text("Add Death Pickup   </>/arrows:Type   up/down:Size   ENTER:Add   DEL:Remove   ESC:Cancel", 58, 160);

    // 7 pickup icons in a row
    const char* names[] = { "Gun","Double","ExtraT","Freeze","1UP","Shield","Claw" };
    int iconX = 60;
    const int iconY = 175;
    for (int i = 0; i < 7; ++i)
    {
        Sprite* spr = getPickupSprite(i, res);
        if (spr)
        {
            if (i == pickupModalType)
            {
                SDL_SetRenderDrawColor(g.getRenderer(), 80, 120, 200, 255);
                SDL_Rect hl = { iconX - 2, iconY - 2, spr->getWidth() + 4, spr->getHeight() + 14 };
                SDL_RenderFillRect(g.getRenderer(), &hl);
            }
            g.draw(spr, iconX, iconY);
            g.setDrawColor(i == pickupModalType ? 255 : 150, 255, 255, 255);
            g.text(names[i], iconX, iconY + spr->getHeight() + 2);
            iconX += spr->getWidth() + 12;
        }
        else
            iconX += 52;
    }

    // Size row (Ball/Hexa only — Glass has no size concept)
    EditorObject* sel = findById(selectedId);
    if (sel && sel->type != StageObjectType::Glass)
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Spawn size: %d", pickupModalSize);
        g.setDrawColor(200, 200, 100, 255);
        g.text(buf, 60, 247);
    }
}

void Editor::drawDeathPickupIcons(const EditorObject& obj)
{
    int count = 0;
    DeathPickupEntry entries[MAX_DEATH_PICKUPS];
    bool isGlass = false;

    if (obj.type == StageObjectType::Ball)
    {
        auto* p = dynamic_cast<const BallParams*>(obj.params.get());
        if (!p || p->deathPickupCount == 0) return;
        count = p->deathPickupCount;
        for (int i = 0; i < count; ++i) entries[i] = p->deathPickups[i];
    }
    else if (obj.type == StageObjectType::Hexa)
    {
        auto* p = dynamic_cast<const HexaParams*>(obj.params.get());
        if (!p || p->deathPickupCount == 0) return;
        count = p->deathPickupCount;
        for (int i = 0; i < count; ++i) entries[i] = p->deathPickups[i];
    }
    else if (obj.type == StageObjectType::Glass)
    {
        auto* p = dynamic_cast<const GlassParams*>(obj.params.get());
        if (!p || !p->hasDeathPickup) return;
        count = 1;
        entries[0] = { 0, p->deathPickupType };
        isGlass = true;
    }
    else return;

    CollisionBox hb = obj.getHitBox();
    StageResources& res = AppData::instance().getStageRes();
    int iconX = hb.x;
    int iconY = hb.y - 2;  // draw above the object to avoid overlap with vel text below

    for (int i = 0; i < count; ++i)
    {
        Sprite* spr = getPickupSprite(static_cast<int>(entries[i].type), res);
        if (!spr) continue;

        int hw = spr->getWidth() / 2;
        int hh = spr->getHeight() / 2;
        if (hw < 1) hw = 1;
        if (hh < 1) hh = 1;

        SDL_Rect dst = { iconX, iconY - hh, hw, hh };
        SDL_RenderCopy(appGraph.getRenderer(), spr->getBmp(), nullptr, &dst);

        if (!isGlass)
        {
            char sz[4];
            std::snprintf(sz, sizeof(sz), "%d", entries[i].size);
            appGraph.setDrawColor(255, 200, 80, 255);
            appGraph.text(sz, iconX + hw / 2, iconY - hh - 8);
        }
        iconX += hw + 3;
    }
}
