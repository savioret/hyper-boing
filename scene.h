#ifndef _SCENE_H_
#define _SCENE_H_

#include <memory>
#include "bmfont.h"
#include "mlist.h"
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

public:
    SceneBitmaps bmp;
    BmNumFont fontNum[3];

    MList lsBalls;
    MList lsItems;
    MList lsFloor;
    MList lsShoots;

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

    void* moveAll() override;
    void checkSequence();

    int release() override;
};

#endif
