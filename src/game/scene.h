#ifndef _SCENE_H_
#define _SCENE_H_

#include <memory>
#include <list>
#include <vector>
#include "bmfont.h"
#include "player.h"
#include "ball.h"
#include "hexa.h"
#include "animeffect.h"
#include "hitscore.h"
#include "shot.h"
#include "spritesheet.h"
#include "floor.h"
#include "glass.h"
#include "platform.h"
#include "ladder.h"
#include "pickup.h"
#include "stage.h"
#include "app.h"
#include "eventmanager.h"
#include "oncehelper.h"
#include "action.h"
#include "freezeeffect.h"
#include "collisionsystem.h"
#include "collisionrules.h"

class StageClear;

/**
 * @enum SceneState
 * @brief Represents the current state of the game scene
 * 
 * The scene progresses through these states in order:
 * Ready -> Playing -> (GameOver | LevelClear)
 */
enum class SceneState
{
    Ready,       ///< "Ready" text blinking, stage is visible but frozen
    Playing,     ///< Normal gameplay, accepting input and running physics
    GameOver,    ///< Game over screen with countdown, allowing continue
    LevelClear   ///< Stage complete, showing StageClear animation
};

/**
 * @enum GameOverSubState
 * @brief Substates for the game over screen
 *
 * Game over has two modes depending on whether continue is available:
 * - ContinueCountdown: Shows 10-second countdown, pressing fire restarts game
 * - Definitive: No continue option, pressing fire returns to menu
 */
enum class GameOverSubState
{
    ContinueCountdown,  ///< Shows countdown (10→0), player can press fire to continue
    Definitive          ///< No continue option, press fire to return to main menu
};

/**
 * @struct SceneBitmaps
 * @brief Contains sprites specific to the current scene
 *
 * This includes the background and stage-specific sprites.
 * Shared sprites (balls, UI elements, weapons, etc.) are stored in AppData.
 */
struct SceneBitmaps
{
    Sprite back;  ///< Stage-specific background image
};

/**
 * @class Scene
 * @brief Core game module managing active gameplay
 * 
 * The Scene class is responsible for:
 * - Managing game entities (balls, players, shots, floors)
 * - Handling collision detection and physics
 * - Processing user input during gameplay
 * - Managing game state transitions (Ready -> Playing -> GameOver/LevelClear)
 * - Rendering the game world and UI elements
 * - Triggering gameplay events (ball hits, player deaths, etc.)
 * 
 * The scene operates as a state machine with four main states:
 * - **Ready**: "Ready" text blinks while stage is displayed
 * - **Playing**: Normal gameplay with full interactivity
 * - **GameOver**: Game over screen with continue countdown
 * - **LevelClear**: Stage complete with score tally
 * 
 * @note Scene inherits from GameState and is part of the game loop
 * @see GameState, Stage, Player, Ball
 */
class Scene : public GameState
{

public:
    // Font indices for fontNum[] array
    static constexpr int FONT_BIG = 0;
    static constexpr int FONT_SMALL = 1;
    static constexpr int FONT_HUGE = 2;

private:
    // Collision detection tolerances (extracted magic numbers)
    static constexpr float GROUND_SNAP_TOLERANCE = 1.0f;    ///< Distance tolerance for snapping to ground
    static constexpr int SURFACE_STEP_TOLERANCE = 2;        ///< Pixel tolerance for stepping onto surfaces
    static constexpr int LADDER_ENTRY_TOLERANCE = 4;        ///< Pixel tolerance for ladder entry detection
    static constexpr int SURFACE_CHECK_WIDTH = 20;          ///< Width used for floor/ladder standing checks

    // Scene state management
    SceneState currentState;           ///< Current scene state (Ready/Playing/GameOver/LevelClear)
    GameOverSubState gameOverSubState; ///< Current game over substate (Continue/Definitive)

    // Stage and level management
    Stage* stage;                           ///< Current stage definition (balls, floors, timing)
    std::unique_ptr<StageClear> pStageClear; ///< Stage clear screen manager
    int gameOverCountdown;                  ///< Countdown timer (10→0 during Continue mode)
    
    // Time management
    int dSecond;         ///< Deciseconds counter (60 = 1 second)
    int timeRemaining;   ///< Time left on stage timer (in seconds)
    int timeLine;        ///< Current timeline position for spawning stage objects

    // Time freeze state (delegated to FreezeEffect helper)
    static constexpr float FREEZE_DURATION = 10.0f;  ///< Default freeze duration (seconds)
    FreezeEffect freezeEffect;                        ///< Encapsulates timer, ball-blink, and countdown

    // FPS and performance tracking
    int moveTick;      ///< SDL tick timestamp for movement updates
    int moveLastTick;  ///< Last movement update timestamp
    int moveCount;     ///< Frame counter for FPS calculation
    int drawTick;      ///< SDL tick timestamp for draw calls
    int drawLastTick;  ///< Last draw call timestamp
    int drawCount;     ///< Draw frame counter
    
    // Debug and utility
    bool boundingBoxes;  ///< Debug mode: show collision bounding boxes

    // Shoot key edge detection (fire only on press, not while held)
    bool shootKeyWasDown[2] = {false, false};
    
    // Ready screen state (time-based using BlinkAction)
    std::unique_ptr<BlinkAction> readyBlinkAction;  ///< Time-based blinking (200ms interval, 13 toggles, ends invisible)
    bool readyVisible;                               ///< Current visibility state for "Ready" text
    
    // Game over input delay (time-based using Action system)
    std::unique_ptr<DelayAction> gameOverInputDelay;  ///< Time-based delay (2 seconds) before accepting input on game over screen (prevents accidental skip)

    // Event subscriptions
    EventManager::ListenerHandle timeWarningHandle;      ///< Subscription for time warning sound
    EventManager::ListenerHandle ballHitHandle;          ///< Subscription for ball hit pop sounds
    EventManager::ListenerHandle playerShootHandle;      ///< Subscription for weapon shoot sounds
    EventManager::ListenerHandle playerHitHandle;        ///< Subscription for player death sound
    EventManager::ListenerHandle pickupCollectedHandle;  ///< Subscription for pickup collected sound
    EventManager::ListenerHandle hitScoreHandle;         ///< Subscription for hit score popups (balls)
    EventManager::ListenerHandle hexaHitScoreHandle;     ///< Subscription for hit score popups (hexas)
    
    // Stage-level utility
    OnceHelper stageOnceHelper;  ///< Helper for one-time actions per stage

    // Font renderer for hit-score popups (system font, owned by Scene)
    BMFontRenderer hitScoreFontRenderer;
    
    // Ball management
    std::vector<std::unique_ptr<Ball>> pendingBalls;  ///< Buffer for balls created from splits

    // Hexa management
    std::vector<std::unique_ptr<Hexa>> pendingHexas;  ///< Buffer for hexas created from splits

    // Collision pipeline
    CollisionSystem collisionSystem;  ///< Detects collisions and resolves physics
    CollisionRules gameRules;              ///< Processes contacts and applies game logic

    /**
     * @brief Validates ball spawn position against floor collisions
     * 
     * Attempts to find a valid position for a ball that doesn't intersect
     * with any existing floors. Only recalculates INT_MAX coordinates.
     * 
     * @param x Ball X coordinate (may be INT_MAX for random)
     * @param y Ball Y coordinate (may be INT_MAX for random)
     * @param ballDiameter Diameter of ball for collision checking
     * @note Tries up to 10 attempts before giving up and spawning anyway
     */
    void checkValidPosition(int& x, int& y, int ballDiameter);

    /**
     * @brief Generic cleanup helper for removing dead objects
     *
     * Iterates through a list, deleting and removing any objects
     * marked as dead (IGameObject::isDead() returns true).
     *
     * @tparam T Object type (must inherit from IGameObject)
     * @param list The list to clean up
     */
    template<typename T>
    void cleanupDeadObjects(std::list<std::unique_ptr<T>>& list);

    /**
     * @brief Clean up dead balls and handle ball splitting
     *
     * Special cleanup for balls that creates child balls when
     * a parent ball is destroyed. Also checks win condition.
     */
    void cleanupBalls();

    /**
     * @brief Clean up dead hexas and handle hexa splitting
     *
     * Special cleanup for hexas that creates child hexas when
     * a parent hexa is destroyed. Also checks win condition.
     */
    void cleanupHexas();

    /**
     * @brief Subscribe to gameplay events
     *
     * Sets up event handlers for: time warnings, ball hits, player shoots, player hits.
     * Called during init().
     */
    void subscribeToEvents();

    /**
     * @brief Unsubscribe from all events
     *
     * Cleans up event subscriptions. Called during release().
     */
    void unsubscribeFromEvents();

    // Helper methods for drawAll()
    /**
     * @brief Draw all game entities (floors, shots, balls)
     */
    void drawEntities();

    /**
     * @brief Draw players with visibility checks
     */
    void drawPlayers();

    /**
     * @brief Draw HUD elements (marks, score, time)
     */
    void drawHUD();

    /**
     * @brief Draw state-specific overlays (game over, ready, stage clear)
     */
    void drawStateOverlay();

    /**
     * @brief Update draw FPS counters
     */
    void updateDrawFPSCounters();

    // Helper methods for moveAll()
    /**
     * @brief Updates FPS counters
     */
    void updateFPSCounters();

    /**
     * @brief Updates all entities (balls, shots, floors)
     * @param dt Delta time in seconds
     */
    void updateEntities(float dt);

    /**
     * @brief Cleanup phase - removes dead objects
     */
    void cleanupPhase();

    /**
     * @brief Updates game timer and timeline
     * @param dt Delta time in seconds
     */
    void updateTimer(float dt);

    /**
     * @brief Updates stage progression and StageClear
     * @param dt Delta time in seconds
     * @return New GameState if transitioning, nullptr otherwise
     */
    GameState* updateStageProgression(float dt);

    // State handlers
    /**
     * @brief Handles Ready state logic
     * 
     * Manages "Ready" text blinking and countdown. Processes time=0
     * stage objects and transitions to Playing state after 6 blinks.
     * 
     * @return nullptr (no state change) or new GameState if transitioning
     */
    GameState* handleReadyState(float dt);
    
    /**
     * @brief Handles Playing state logic
     * 
     * Processes player input, updates entities, checks collisions,
     * manages timer, and handles game over/level clear conditions.
     * 
     * @param dt Delta time in seconds
     * @return nullptr (no state change) or new GameState if transitioning
     */
    GameState* handlePlayingState(float dt);
    
    /**
     * @brief Handles GameOver state logic
     * 
     * Displays game over screen with continue countdown and allows
     * player to restart or return to menu.
     * 
     * @param dt Delta time in seconds
     * @return nullptr (stay in scene) or new GameState (Menu/Scene)
     */
    GameState* handleGameOverState(float dt);
    
    /**
     * @brief Handles LevelClear state logic
     * 
     * Manages StageClear animation and transitions to next stage
     * or back to menu when complete.
     * 
     * @param dt Delta time in seconds
     * @return nullptr (stay in scene) or new GameState (Scene/Menu)
     */
    GameState* handleLevelClearState(float dt);
    
    /**
     * @brief Transitions scene to a new state
     * 
     * Updates the currentState and performs any necessary cleanup
     * or initialization for the new state.
     * 
     * @param newState The state to transition to
     */
    void setState(SceneState newState);

public:
    SceneBitmaps bmp;     ///< Scene-specific sprites (background, weapons)
    BmNumFont fontNum[3]; ///< Bitmap fonts (small, medium, large)

    // Entity lists
    std::list<std::unique_ptr<Ball>> lsBalls;       ///< Active balls in scene
    std::list<std::unique_ptr<Hexa>> lsHexas;       ///< Active hexas in scene
    std::list<std::unique_ptr<Pickup>> lsPickups;   ///< Active pickups in scene
    std::list<std::unique_ptr<Platform>> lsFloor;   ///< Active platforms (Floor and Glass)
    std::list<std::unique_ptr<Ladder>> lsLadders;   ///< Active ladders (climbable)
    std::list<std::unique_ptr<Shot>> lsShoots;      ///< Active weapon shots
    std::list<std::unique_ptr<AnimEffect>> lsEffects; ///< Active one-shot animations (pops, sparks)
    std::list<std::unique_ptr<HitScore>> lsHitScores; ///< Floating score popups (ball hits)

    /**
     * @brief Constructs a new Scene for the given stage
     * 
     * @param stg Stage definition containing layout and timeline
     * @param pstgclr Optional StageClear object (for stage transitions)
     */
    Scene(Stage* stg, std::unique_ptr<StageClear> pstgclr = nullptr);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Scene() {}
    
    /**
     * @brief Adds a ball to the scene
     * 
     * @param x X position (INT_MAX for random)
     * @param y Y position (INT_MAX for random)
     * @param size Ball size (0=largest, 3=smallest)
     * @param top Maximum jump height (0=auto)
     * @param dirX Horizontal direction (-1, 0, 1)
     * @param dirY Vertical direction (-1, 1)
     * @param id Ball type/color (0=red)
     */
    void addBall(int x = 250, int y = -20, int size = 0, int top = 0, int dirX = 1, int dirY = 1, int id = 0);
    
    /**
     * @brief Adds a pickup to the scene
     * @param x X position (center)
     * @param y Y position (bottom)
     * @param type Pickup type
     */
    void addPickup(int x, int y, PickupType type);

    /**
     * @brief Adds a hexa to the scene
     * @param x X position (top-left)
     * @param y Y position (top-left)
     * @param size Hexa size (0=large, 1=medium, 2=small)
     * @param velX Horizontal velocity
     * @param velY Vertical velocity
     */
    void addHexa(int x, int y, int size, float velX, float velY);

    /**
     * @brief Adds a floor/platform to the scene
     * @param x X position
     * @param y Y position
     * @param type Floor shape variant
     */
    void addFloor(int x, int y, FloorType type);

    /**
     * @brief Adds a glass platform to the scene
     * @param x X position
     * @param y Y position
     * @param type Glass shape variant
     */
    void addGlass(int x, int y, GlassType type);

    /**
     * @brief Adds a ladder to the scene
     * @param x X position (center, bottom-middle anchor)
     * @param y Y position (bottom of ladder)
     * @param numTiles Number of vertical tiles
     */
    void addLadder(int x, int y, int numTiles);

    /**
     * @brief Gets the list of ladders
     * @return Const reference to ladder list
     */
    const std::list<std::unique_ptr<Ladder>>& getLadders() const { return lsLadders; }

    /**
     * @brief Finds a ladder that the player can enter (50% overlap)
     * @param player The player to check
     * @return Pointer to ladder if found, nullptr otherwise
     */
    Ladder* findLadderAtPlayer(Player* player) const;

    /**
     * @brief Finds a ladder below the player (for climbing down from platforms)
     * @param player The player to check
     * @return Pointer to ladder if found, nullptr otherwise
     */
    Ladder* findLadderBelowPlayer(Player* player) const;

    /**
     * @brief Finds a platform directly under the player's feet
     * @param player The player to check
     * @return Pointer to platform if found, nullptr otherwise
     */
    Platform* findFloorUnderPlayer(Player* player) const;

    /**
     * @brief Finds the top of a ladder under the player's feet
     * @param player The player to check
     * @return Pointer to ladder if found, nullptr otherwise
     */
    Ladder* findLadderTopUnderPlayer(Player* player) const;

    /**
     * @brief Finds the ground level below the player (floor, ladder top, or MAX_Y)
     * @param player The player to check
     * @return Y coordinate of the nearest surface below the player
     */
    float findGroundLevel(Player* player) const;

    /**
     * @brief Finds a short (≤16 px tall) floor platform the player is walking into from the side
     * @param player The player to check (must be grounded)
     * @return Pointer to steppable platform if found, nullptr otherwise
     */
    Platform* findSteppablePlatformInPath(Player* player) const;

    /**
     * @brief Spawns a one-shot animation effect centered at (x, y)
     * @param tmpl Template AnimSpriteSheet to clone (must not be null)
     * @param x Center X in screen coordinates
     * @param y Center Y in screen coordinates
     * @param scale Scale factor (1.0 = full size, 0.5 = half size, etc.)
     */
    void spawnEffect(AnimSpriteSheet* tmpl, int x, int y, float scale = 1.0f);

    /**
     * @brief Fires a shot from the player
     * 
     * Creates weapon projectile(s) based on player's current weapon
     * and triggers PLAYER_SHOOT event.
     * 
     * @param pl Player firing the shot
     */
    void shoot(Player* pl);
    
    /**
     * @brief Gets a player by index
     * @param index Player index (0 or 1)
     * @return Pointer to player if active and playing, nullptr otherwise
     */
    Player* getPlayer(int index);

    /**
     * @brief Creates a shot of the appropriate type based on weapon
     * 
     * Factory method that instantiates the correct Shot subclass
     * (HarpoonShot, GunShot, etc.) based on weapon type.
     * 
     * @param pl Player firing the shot
     * @param type Weapon type
     * @return unique_ptr to created Shot object
     */
    std::unique_ptr<Shot> createShot(Player* pl, WeaponType type);

    /**
     * @brief Triggers level win sequence
     * 
     * Stops music, plays win music, and creates StageClear screen.
     * Fires LEVEL_CLEAR event.
     */
    void win();
    
    /**
     * @brief Skips to a specific stage (debug command)
     * @param stageNumber Stage to skip to (1-indexed)
     */
    void skipToStage(int stageNumber);
    
    /**
     * @brief Initializes the scene
     * 
     * Loads bitmaps, music, resets counters, and fires STAGE_LOADED event.
     * 
     * @return 1 on success
     */
    int init() override;
    
    /**
     * @brief Loads all scene-specific bitmaps
     * 
     * Initializes background, fonts, and weapon sprites.
     * Shared resources are loaded from AppData.
     * 
     * @return 1 on success
     */
    int initBitmaps();
    
    /**
     * @brief Draws the stage background
     */
    void drawBackground();
    
    /**
     * @brief Main draw function for the scene
     * 
     * Renders all entities, UI, and debug overlays in proper order.
     * 
     * @return 1 on success
     */
    int drawAll() override;
    
    /**
     * @brief Draws a ball sprite
     * @param b Ball to draw
     */
    void draw(Ball* b);

    /**
     * @brief Draws a hexa sprite
     * @param h Hexa to draw
     */
    void draw(Hexa* h);
    
    /**
     * @brief Draws a player sprite
     * @param pl Player to draw
     */
    void draw(Player* pl);
    
    /**
     * @brief Draws a platform sprite (Floor or Glass)
     * @param pl Platform to draw
     */
    void draw(Platform* pl);
    
    /**
     * @brief Draws player scores and lives UI
     */
    void drawScore();
    
    /**
     * @brief Draws border markers around play area
     */
    void drawMark();
    
    /**
     * @brief Draws collision bounding boxes (debug mode)
     */
    void drawBoundingBoxes();
    
    /**
     * @brief Draws debug overlay with performance and entity info
     */
    void drawDebugOverlay() override;

    /**
     * @brief Main update function for the scene
     * 
     * Updates all entities, handles input, checks collisions,
     * manages state transitions, and returns next state if changing.
     * 
     * @param dt Delta time in seconds
     * @return nullptr to stay in scene, or new GameState to transition
     */
    GameState* moveAll(float dt) override;

    /**
     * @brief Enables/disables debug bounding box visualization
     * @param enabled True to show bounding boxes
     */
    void setBoundingBoxes(bool enabled) { boundingBoxes = enabled; }
    
    /**
     * @brief Gets debug bounding box state
     * @return True if bounding boxes are enabled
     */
    bool getBoundingBoxes() const { return boundingBoxes; }

    /**
     * @brief Sets the stage countdown timer
     * @param seconds Time remaining in seconds
     */
    void setTimeRemaining(int seconds) { timeRemaining = seconds; }

    /**
     * @brief Gets the stage countdown timer
     * @return Time remaining in seconds
     */
    int getTimeRemaining() const { return timeRemaining; }

    /**
     * @brief Sets the time freeze state (from TIME_FREEZE pickup)
     * @param frozen True to freeze balls, false to unfreeze
     * @param duration Duration of freeze in seconds (only used when frozen=true)
     */
    void setTimeFrozen(bool frozen, float duration = FREEZE_DURATION);

    /**
     * @brief Gets the time freeze state
     * @return True if time is frozen
     */
    bool isTimeFrozen() const { return freezeEffect.isActive(); }

    /**
     * @brief Processes stage timeline to spawn objects
     * 
     * Checks if any stage objects (balls, floors, actions) should
     * spawn at the current timeline position and creates them.
     */
    void checkSequence();

    /**
     * @brief Stage-level one-time action helper
     * 
     * Ensures an action only happens once per stage (resets on stage change).
     * 
     * @param key Unique identifier for the action
     * @return True if this is the first call with this key, false otherwise
     */
    bool once(const std::string& key) {
        return stageOnceHelper.once(key);
    }

    /**
     * @brief Gets the stage-level once helper
     * @return Reference to the OnceHelper instance
     */
    OnceHelper& getOnceHelper() { return stageOnceHelper; }

    /**
     * @brief Releases scene resources
     * 
     * Deletes all entities, releases scene-specific sprites,
     * and stops music.
     * 
     * @return 1 on success
     */
    int release() override;
};

// Template implementation must be in header for C++14
template<typename T>
void Scene::cleanupDeadObjects(std::list<std::unique_ptr<T>>& list)
{
    list.remove_if([](const std::unique_ptr<T>& obj) {
        return obj->isDead();
    });
}

#endif
