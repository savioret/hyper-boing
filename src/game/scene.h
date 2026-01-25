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
 * FloorColision class
 * Auxiliary class for handling collisions between balls and floors.
 */
class FloorColision
{
public:
    Floor* floor;
    SDL_Point point;

    FloorColision() : floor(nullptr) { point.x = point.y = 0; }
};

/**
 * SceneBitmaps struct
 * Contains sprites for the game scene.
 */
struct SceneBitmaps
{
    Sprite back;
    Sprite redball[4];
    Sprite floor[2];
    Sprite shoot[3];  // Keep for backward compatibility during transition
    Sprite mark[5];
    Sprite fontnum[3];
    Sprite miniplayer[2];
    Sprite lives[2];
    Sprite gameover;
    Sprite continu;
    Sprite time;
    Sprite ready;

    // Weapon-specific sprites
    struct
    {
        Sprite harpoonHead;
        Sprite harpoonTail1;
        Sprite harpoonTail2;
        SpriteSheet gunBullet;
    } weapons;
};

/**
 * Scene class
 *
 * This is the core game module. It manages the active gameplay:
 * balls, players, shoots, floors, and collision logic.
 * Inherits from GameState.
 */
class Scene : public GameState
{
    friend class Ball;
private:
    bool levelClear;
    std::unique_ptr<StageClear> pStageClear;
    bool gameOver;
    int gameOverCount;
    Stage* stage;
    
    int change;
    int dSecond; // deciseconds counter
    int timeRemaining;
    int timeLine;
    
    // FPS timing variables (previously static in moveAll and drawAll)
    int moveTick;
    int moveLastTick;
    int moveCount;
    int drawTick;
    int drawLastTick;
    int drawCount;

    bool boundingBoxes;  // Debug mode: show bounding boxes for all collidable objects

    // Ready screen state
    bool readyActive;        // Whether ready screen is currently active
    int readyBlinkCount;     // Number of blinks so far (0-6)
    int readyBlinkTimer;     // Timer for blink intervals (200ms = ~12 frames at 60fps)
    bool readyVisible;       // Current visibility state for blinking

    // Time warning (plays sound once when time reaches 11 seconds)
    EventManager::ListenerHandle timeWarningHandle;

    // Stage-level "once" helper (resets every stage)
    OnceHelper stageOnceHelper;

    std::vector<Ball*> pendingBalls;  // Buffer to hold new balls to be added
    
    /**
     * Check if ball position is valid (no collision with floors)
     * Adjusts coordinates if collision detected (up to 10 attempts)
     * Only recalculates INT_MAX coordinates (random positions)
     * @param x Ball X coordinate (may be INT_MAX)
     * @param y Ball Y coordinate (may be INT_MAX)
     * @param ballDiameter Diameter of ball for collision check
     */
    void checkValidPosition(int& x, int& y, int ballDiameter);

    /**
     * Generic cleanup helper for dead objects.
     * Removes and deletes objects marked as dead from a list.
     * 
     * @tparam T Object type (must inherit from IGameObject)
     * @param list The list to clean up
     */
    template<typename T>
    void cleanupDeadObjects(std::list<T*>& list);
    
    void processBallDivisions(); // Handles ball divisions after collisions
    void splitBall(Ball* ball); // Queue a ball to split into smaller balls

public:
    SceneBitmaps bmp;
    BmNumFont fontNum[3];

    std::list<Ball*> lsBalls;
    std::list<Item*> lsItems;
    std::list<Floor*> lsFloor;
    std::list<Shot*> lsShoots;  // Changed from Shoot* to Shot* for polymorphism

    Scene(Stage* stg, std::unique_ptr<StageClear> pstgclr = nullptr);
    virtual ~Scene() {}
    
    void addBall(int x = 250, int y = -20, int size = 0, int top = 0, int dirX = 1, int dirY = 1, int id = 0);
    void addItem(int x, int y, int id);
    void addShoot(Player* pl);
    void addFloor(int x, int y, int id);
    void shoot(Player* pl);
    Player* getPlayer(int index);  // Get player by index (0 or 1)

    /**
     * Create a shot of the appropriate type based on weapon type
     * @param pl Player firing the shot
     * @param type Weapon type
     * @param xOffset Horizontal offset for multi-projectile weapons
     * @return Pointer to created Shot object
     */
    Shot* createShot(Player* pl, WeaponType type, int xOffset);

    int divideBall(Ball* b);
    void checkColisions();
    void decide(Ball* b, FloorColision* fc, int moved);

    int objectScore(int id);
    void win();
    void skipToStage(int stageNumber);
    
    int init() override;
    int initBitmaps();
    void drawBackground();
    int drawAll() override;
    void draw(Ball* b);
    void draw(Player* pl);
    void draw(Floor* fl);
    void drawScore();
    void drawMark();
    void drawBoundingBoxes();
    void drawDebugOverlay() override;

    GameState* moveAll(float dt) override;

    // Debug accessors
    void setBoundingBoxes(bool enabled) { boundingBoxes = enabled; }
    bool getBoundingBoxes() const { return boundingBoxes; }
    void checkSequence();

    // Stage-level "once" helper (resets every stage)
    bool once(const std::string& key) {
        return stageOnceHelper.once(key);
    }

    OnceHelper& getOnceHelper() { return stageOnceHelper; }

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
