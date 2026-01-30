#ifndef _SCENE_H_
#define _SCENE_H_

#include <memory>
#include <list>
#include <vector>
#include "bmfont.h"
#include "player.h"
#include "ball.h"
#include "shot.h"
#include "spritesheet.h"
#include "floor.h"
#include "item.h"
#include "stage.h"
#include "app.h"
#include "eventmanager.h"
#include "oncehelper.h"

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
 * @class FloorColision
 * @brief Auxiliary class for handling collisions between balls and floors
 * 
 * Stores collision information when a ball intersects with a floor,
 * including which floor was hit and at what point.
 */
class FloorColision
{
public:
    Floor* floor;      ///< Pointer to the floor involved in collision
    SDL_Point point;   ///< Collision point coordinates

    FloorColision() : floor(nullptr) { point.x = point.y = 0; }
};

/**
 * @struct SceneBitmaps
 * @brief Contains sprites specific to the current scene
 * 
 * This includes the background and stage-specific sprites.
 * Shared sprites (balls, UI elements, etc.) are stored in AppData.
 */
struct SceneBitmaps
{
    Sprite back;  ///< Stage-specific background image

    // Weapon-specific sprites (loaded per scene for now)
    struct
    {
        Sprite harpoonHead;
        Sprite harpoonTail1;
        Sprite harpoonTail2;
        SpriteSheet gunBullet;
    } weapons;
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
    friend class Ball;
    
private:
    // Scene state management
    SceneState currentState;           ///< Current scene state (Ready/Playing/GameOver/LevelClear)
    GameOverSubState gameOverSubState; ///< Current game over substate (Continue/Definitive)

    // Stage and level management
    Stage* stage;                           ///< Current stage definition (balls, floors, timing)
    std::unique_ptr<StageClear> pStageClear; ///< Stage clear screen manager
    bool levelClear;                        ///< True when all balls are destroyed
    bool gameOver;                          ///< True when all players are dead or time expired
    int gameOverCount;                      ///< Countdown timer (10→0 during ContinueCountdown, -2=inactive)
    
    // Time management
    int dSecond;         ///< Deciseconds counter (60 = 1 second)
    int timeRemaining;   ///< Time left on stage timer (in seconds)
    int timeLine;        ///< Current timeline position for spawning stage objects
    
    // FPS and performance tracking
    int moveTick;      ///< SDL tick timestamp for movement updates
    int moveLastTick;  ///< Last movement update timestamp
    int moveCount;     ///< Frame counter for FPS calculation
    int drawTick;      ///< SDL tick timestamp for draw calls
    int drawLastTick;  ///< Last draw call timestamp
    int drawCount;     ///< Draw frame counter
    
    // Debug and utility
    bool boundingBoxes;  ///< Debug mode: show collision bounding boxes
    
    // Ready screen state
    int readyBlinkCount;   ///< Number of blinks completed (0-6)
    int readyBlinkTimer;   ///< Timer for blink intervals (12 frames = 200ms at 60fps)
    bool readyVisible;     ///< Current visibility state for "Ready" text
    
    // Event subscriptions
    EventManager::ListenerHandle timeWarningHandle;  ///< Subscription for time warning sound
    
    // Stage-level utility
    OnceHelper stageOnceHelper;  ///< Helper for one-time actions per stage
    
    // Ball management
    std::vector<Ball*> pendingBalls;  ///< Buffer for balls created from splits
    
    // Legacy (deprecated)
    int change;  ///< Unused legacy variable (TODO: remove)
    
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
    void cleanupDeadObjects(std::list<T*>& list);
    
    /**
     * @brief Handles ball divisions after collisions
     * 
     * Processes the pending ball queue, adding new balls created
     * from splits to the active ball list.
     */
    void processBallDivisions();
    
    /**
     * @brief Queues a ball to split into smaller balls
     * 
     * Creates two child balls from a parent ball and queues them
     * for addition after cleanup phase.
     * 
     * @param ball Parent ball to split
     */
    void splitBall(Ball* ball);
    
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
    std::list<Ball*> lsBalls;    ///< Active balls in scene
    std::list<Item*> lsItems;    ///< Active items (unused currently)
    std::list<Floor*> lsFloor;   ///< Active floors/platforms
    std::list<Shot*> lsShoots;   ///< Active weapon shots

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
     * @brief Adds an item to the scene
     * @param x X position
     * @param y Y position
     * @param id Item type
     */
    void addItem(int x, int y, int id);
    
    /**
     * @brief Adds a shot from the player (deprecated, use shoot() instead)
     * @param pl Player firing the shot
     */
    void addShoot(Player* pl);
    
    /**
     * @brief Adds a floor/platform to the scene
     * @param x X position
     * @param y Y position
     * @param id Floor type (0=horizontal, 1=vertical)
     */
    void addFloor(int x, int y, int id);
    
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
     * @param xOffset Horizontal offset for multi-projectile weapons
     * @return Pointer to created Shot object
     */
    Shot* createShot(Player* pl, WeaponType type, int xOffset);

    /**
     * @brief Handles ball hit logic (split or destroy)
     * 
     * @param b Ball that was hit
     * @return 1 if ball destroyed, 0 if split
     * @deprecated Use splitBall() instead
     */
    int divideBall(Ball* b);
    
    /**
     * @brief Checks all collision pairs (balls/players/shots/floors)
     * 
     * Performs collision detection and response for:
     * - Balls vs Shots (destroy ball)
     * - Balls vs Floors (bounce)
     * - Balls vs Players (kill player)
     * - Shots vs Floors (destroy shot)
     */
    void checkColisions();
    
    /**
     * @brief Resolves complex ball-floor collision scenarios
     * 
     * Called when a ball collides with multiple floors simultaneously
     * to determine the correct bounce direction.
     * 
     * @param b Ball in collision
     * @param fc Array of floor collision data (size 2)
     * @param moved Direction of movement (1=X, 2=Y)
     */
    void decide(Ball* b, FloorColision* fc, int moved);

    /**
     * @brief Calculates score for destroying an object
     * @param id Object size/type
     * @return Score value
     */
    int objectScore(int id);
    
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
     * @brief Draws a player sprite
     * @param pl Player to draw
     */
    void draw(Player* pl);
    
    /**
     * @brief Draws a floor sprite
     * @param fl Floor to draw
     */
    void draw(Floor* fl);
    
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
void Scene::cleanupDeadObjects(std::list<T*>& list)
{
    for (auto it = list.begin(); it != list.end(); )
    {
        if ((*it)->isDead())
        {
            delete *it;
            it = list.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

#endif
