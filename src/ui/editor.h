#pragma once

#include <vector>
#include <memory>
#include <string>
#include <SDL.h>
#include "../core/app.h"
#include "../game/stage.h"
#include "../core/collisionbox.h"
#include "../core/stageresources.h"

/**
 * EditorObject - Lightweight proxy for stage objects in the editor.
 *
 * Does NOT instantiate actual game entities (Ball, Hexa, etc.) because
 * those require Scene* and run physics. Instead, stores the same data
 * as StageObjectParams and renders directly from StageResources sprites.
 */
struct EditorObject
{
    int id = 0;                                  ///< Unique ID within editor session
    StageObjectType type = StageObjectType::Null; ///< Object type
    float x = 0, y = 0;                         ///< Logical position (same coord space as .stg)
    int startTime = 0;                           ///< Spawn time in timeline
    std::unique_ptr<StageObjectParams> params;   ///< Type-specific params (BallParams, FloorParams, etc.)
    bool selected = false;
    int spriteW = 0, spriteH = 0;               ///< Cached sprite dimensions

    EditorObject() = default;
    EditorObject(EditorObject&&) = default;
    EditorObject& operator=(EditorObject&&) = default;

    // No copy - params is unique_ptr
    EditorObject(const EditorObject&) = delete;
    EditorObject& operator=(const EditorObject&) = delete;

    /// Get a screen-space bounding box for hit-testing
    CollisionBox getHitBox() const;
};

/**
 * Editor - Visual stage editor mode.
 *
 * A GameState that renders stage objects as static sprites and allows
 * placing, selecting, dragging, and property-editing objects via mouse
 * and keyboard. No physics or collision logic runs.
 *
 * Enter via Ctrl+E from Scene; exit via Ctrl+E back to Scene.
 */
class Editor : public GameState
{
public:
    explicit Editor(Stage* stage);
    ~Editor() override;

    int init() override;
    GameState* moveAll(float dt) override;
    int drawAll() override;
    int release() override;
    void onSDLEvent(const SDL_Event& e) override;

    Stage* getStage() const { return stage; }
    bool isPlacing() const { return floatingObj != nullptr; }
    void writeBackToStage();

private:
    // --- Data ---
    Stage* stage;                               ///< Non-owning pointer to AppData stage
    std::vector<EditorObject> objects;           ///< All placed objects
    int nextId = 1;

    // Player positions (always visible, draggable)
    float playerX[2], playerY[2];
    int draggedPlayer = -1;                     ///< -1=none, 0=P1, 1=P2

    // Selection
    int selectedId = -1;                        ///< ID of selected object (-1=none)

    // Mouse state
    int mouseX = 0, mouseY = 0;
    bool mouseDown = false;
    bool isDragging = false;
    int dragOffsetX = 0, dragOffsetY = 0;

    // Floating object (being placed, follows mouse until click)
    std::unique_ptr<EditorObject> floatingObj;

    // Last cloned object template (for repeated C-key placement)
    std::unique_ptr<EditorObject> cloneTemplate;

    // UI toggles
    bool showBBoxes = true;
    int gridMode = 1; // 1=8px, 2=4px, 0=off
    bool dirty = false;

    // Status bar message
    std::string statusMsg;
    float statusTimer = 0.0f;

    // Pickup modal
    bool showingPickupModal = false;
    int pickupModalType = 0;   ///< PickupType cast to int (0-6)
    int pickupModalSize = 0;   ///< Spawn size (Ball/Hexa only)

    // Background
    Sprite backgroundSprite;


    // --- Input handlers ---
    void onMouseDown(int mx, int my, int button);
    void onMouseUp(int mx, int my, int button);
    void onMouseMove(int mx, int my);
    void onMouseWheel(int direction);
    void onKeyDown(SDL_Keycode key, Uint16 mod);

    // --- Object operations ---
    EditorObject* findObjectAt(int mx, int my);
    EditorObject* findById(int id);
    void selectObject(int id);
    void deselectAll();
    void deleteSelected();
    void startPlacing(StageObjectType type);
    void placeFloatingObject();

    // --- Property cycling ---
    void cycleSizeOrType(EditorObject& obj, int dir);
    void cycleColor(EditorObject& obj, int dir);

    // --- Sprite lookup ---
    Sprite* getSpriteForObject(const EditorObject& obj) const;
    void updateCachedDimensions(EditorObject& obj);

    // --- Drawing ---
    void drawBackground();
    void drawPlayfieldBorder();
    void drawObjects();
    void drawObject(const EditorObject& obj);
    void drawPlayers();
    void drawFloatingObject();
    void drawSelectionHighlight(const EditorObject& obj);
    void drawBoundingBox(const EditorObject& obj);
    void drawStatusBar();
    void drawVelocityArrow(const EditorObject& obj);
    void drawPickupModal();
    void drawDeathPickupIcons(const EditorObject& obj);

    // --- Pickup modal ---
    void openPickupModal();
    void confirmPickupModal();
    void removeLastPickup();
    Sprite* getPickupSprite(int typeInt, StageResources& res) const;

    // --- Save/Load ---
    void loadFromStage();
    void saveToFile();

    // --- Helpers ---
    void setStatus(const std::string& msg);
    bool isBottomMiddleType(StageObjectType type) const;
    CollisionBox getPlayerHitBox(int idx) const;

    /// Convert SDL window coordinates to logical game coordinates
    void windowToLogical(int winX, int winY, int& outX, int& outY);
};
