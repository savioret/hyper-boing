#include "hexa.h"
#include "../main.h"
#include "animspritesheet.h"
#include "animcontroller.h"
#include "eventmanager.h"
#include "../game/collisionsystem.h"
#include <cmath>

// Frame ranges in hexa_green.json per size
static constexpr int HEXA_ANIM_START[] = {0, 4, 8};  // size 0=big, 1=medium, 2=small

// Static member initialization
constexpr int Hexa::SIZES[][2];

Hexa::Hexa(Scene* scn, int x, int y, int sz, float vx, float vy, int col)
    : scene(scn), size(sz), color(static_cast<Color>(col)), velX(vx), velY(vy)
{
    this->xPos = (float)x;
    this->yPos = (float)y;

    // Clamp size to valid range
    if (size < 0) size = 0;
    if (size > 2) size = 2;

    width = SIZES[size][0];
    height = SIZES[size][1];

    int start = HEXA_ANIM_START[size];
    animCtrl = std::make_unique<FrameSequenceAnim>(FrameSequenceAnim::range(start, start + 3, 100, 0));
}

Hexa::Hexa(Scene* scn, Hexa* parent, int dirX, int dirY)
    : scene(scn), color(parent->color)
{
    // Child is one size smaller
    size = parent->size + 1;
    if (size > 2) size = 2;

    width = SIZES[size][0];
    height = SIZES[size][1];

    // Horizontal: spread outward using parent's horizontal speed magnitude
    velX = dirX * std::abs(parent->velX);
    if (velX == 0.0f) velX = dirX * 1.5f;  // Default if parent had no horizontal movement

    // Vertical: apply dirY to parent's vertical speed magnitude
    velY = dirY * std::abs(parent->velY);
    if (velY == 0.0f) velY = dirY * 1.0f;  // Default if parent had no vertical movement

    // Position at parent's center, offset horizontally by direction
    float parentCenterX = parent->xPos + (parent->width / 2.0f);
    float parentCenterY = parent->yPos + (parent->height / 2.0f);

    this->xPos = parentCenterX - (width / 2.0f) + (dirX * width * 0.5f);
    this->yPos = parentCenterY - (height / 2.0f);

    int start = HEXA_ANIM_START[size];
    animCtrl = std::make_unique<FrameSequenceAnim>(FrameSequenceAnim::range(start, start + 3, 100, 0));
}

void Hexa::update(float dt)
{
    // Handle flash state (countdown before actual death)
    if (flashing)
    {
        flashTimer -= dt;
        if (flashTimer <= 0.0f)
        {
            // Flash complete - now actually die
            IGameObject::kill();  // Call parent to set dead=true
        }
        return;  // Don't move during flash
    }

    // Advance frame animation (dt is in seconds, animCtrl expects milliseconds)
    animCtrl->update(dt * 1000.0f);

    // Constant velocity movement
    xPos += velX;
    yPos += velY;

    // Screen edge bouncing
    if (velX > 0 && xPos + width >= Stage::MAX_X)
    {
        xPos = (float)(Stage::MAX_X - width);
        velX = -velX;
    }
    else if (velX < 0 && xPos <= Stage::MIN_X)
    {
        xPos = (float)Stage::MIN_X;
        velX = -velX;
    }

    if (velY > 0 && yPos + height >= Stage::MAX_Y+1)
    {
        yPos = (float)(Stage::MAX_Y - height);
        velY = -velY;
    }
    else if (velY < 0 && yPos <= Stage::MIN_Y)
    {
        yPos = (float)Stage::MIN_Y;
        velY = -velY;
    }
}

void Hexa::kill()
{
    if (!flashing && !dead)
    {
        flashing = true;
        flashTimer = FLASH_DURATION;
        // onDeath() is called by IGameObject::kill() when the flash expires,
        // ensuring it fires exactly once (splash effect, events, pickup spawn).
    }
}

void Hexa::onDeath()
{
    // Spawn splash animation centered on this hexa with size-based scaling
    // Size 0 = 100%, Size 1 = 50%, Size 2 = 33%
    static const float sizeScales[] = { 1.0f, 0.5f, 0.33f };
    float scale = (size >= 0 && size <= 2) ? sizeScales[size] : 0.33f;

    StageResources& res = gameinf.getStageRes();
    AnimSpriteSheet* tmpl = res.getHexaSplashAnim(static_cast<int>(color));
    if (tmpl)
    {
        int centerX = (int)xPos + width / 2;
        int centerY = (int)yPos + height / 2;
        scene->spawnEffect(tmpl, centerX, centerY + (int)(tmpl->getHeight() * scale / 2), scale);
    }

    // Fire HEXA_SPLIT event if hexa will split into smaller hexas
    if (size < 2)
    {
        GameEventData event(GameEventType::HEXA_SPLIT);
        event.hexaSplit.parentHexa = this;
        event.hexaSplit.parentSize = size;
        EVENT_MGR.trigger(event);
    }

    // Spawn only the death pickup whose size matches this hexa's current size.
    for (int i = 0; i < deathPickupCount; i++)
    {
        if (deathPickups[i].size == size)
        {
            int cx = (int)xPos + width / 2;
            int cy = (int)yPos + height / 2;
            scene->addPickup(cx, cy, deathPickups[i].type);
            break;  // At most one pickup per size level
        }
    }
}

void Hexa::handlePlatformBounce(const CollisionSide& side)
{
    // Reflect velocity based on collision side
    if (side.hasHorizontal())
    {
        velX = -velX;
    }
    if (side.hasVertical())
    {
        velY = -velY;
    }
}

std::pair<std::unique_ptr<Hexa>, std::unique_ptr<Hexa>> Hexa::createChildren()
{
    // Only create children if hexa is large enough to split
    if (size >= 2)
    {
        return { nullptr, nullptr };
    }

    // Children inherit parent's vertical direction:
    // - parent going down → children go down-left and down-right
    // - parent going up   → children go up-left and up-right
    int childDirY = -1;//(velY >= 0.0f) ? 1 : -1;

    auto child1 = std::make_unique<Hexa>(scene, this, -1, childDirY);  // Left
    auto child2 = std::make_unique<Hexa>(scene, this,  1, childDirY);  // Right

    // Propagate pickups bound to sizes beyond this hexa to one random child.
    Hexa* lucky = (rand() % 2 == 0) ? child1.get() : child2.get();
    for (int i = 0; i < deathPickupCount; i++)
    {
        if (deathPickups[i].size > size)
            lucky->addDeathPickup(deathPickups[i]);
    }

    return { std::move(child1), std::move(child2) };
}

bool Hexa::collision(Shot* sh)
{
    if (sh->isDead())
        return false;

    return intersects(getCollisionBox(), sh->getCollisionBox());
}

bool Hexa::collision(Platform* fl)
{
    return intersects(getCollisionBox(), fl->getCollisionBox());
}

bool Hexa::collision(Player* pl)
{
    return intersects(getCollisionBox(), pl->getCollisionBox());
}

Sprite* Hexa::getCurrentSprite() const
{
    StageResources& res = gameinf.getStageRes();
    AnimSpriteSheet* anim = res.getHexaAnim(static_cast<int>(color));
    return (anim && animCtrl) ? anim->getFrame(animCtrl->getCurrentFrame()) : nullptr;
}
