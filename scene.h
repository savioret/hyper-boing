#ifndef _SCENE_H_
#define _SCENE_H_

#include <memory>
#include <list>
#include <vector>
#include "bmfont.h"
#include "player.h"
#include "ball.h"
#include "shoot.h"
#include "floor.h"
#include "item.h"
#include "stage.h"
#include "app.h"

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
    Sprite shoot[3];
    Sprite mark[5];
    Sprite fontnum[3];
    Sprite miniplayer[2];
    Sprite lives[2];
    Sprite gameover;
    Sprite continu;
    Sprite time;
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

    std::vector<Ball*> pendingBalls;  // Buffer to hold new balls to be added

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
    std::list<Shoot*> lsShoots;

    Scene(Stage* stg, std::unique_ptr<StageClear> pstgclr = nullptr);
    virtual ~Scene() {}
    
    void addBall(int x = 250, int y = -20, int size = 0, int top = 0, int dirX = 1, int dirY = 1, int id = 0);
    void addItem(int x, int y, int id);
    void addShoot(Player* pl);
    void addFloor(int x, int y, int id);
    void shoot(Player* pl);

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
    void draw(Shoot* sht);
    void draw(Floor* fl);
    void drawScore();
    void drawMark();
    void drawDebugOverlay() override;

    GameState* moveAll() override;
    void checkSequence();

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
