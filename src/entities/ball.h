#pragma once

#include <SDL.h>
#include "gameobject.h"

class Scene;
class Shot;
class Player;
class Floor;
class Sprite;

/**
 * Ball class
 *
 * It is the game ball, which must be shot.
 */
class Ball : public IGameObject
{
private:
    Sprite* sprite;
    int top; // maximum height from the floor
    int diameter;
    int size;
    int id;

    int dirX, dirY; // direction
    float time, maxTime;
    float y0; // initial space
    float gravity; // acceleration (acc)
    Scene* scene;

public:
    Ball(Scene* scene, Ball* oldBall);
    Ball(Scene* scene, int x, int y, int size, int dirX = 1, int dirY = 1, int top = 0, int id = 0);
    ~Ball();

    void init();
    void initTop();

    void update(float dt);
    
    // IGameObject lifecycle hook
    void onDeath() override;
    
    bool collision(Shot* shot);
    SDL_Point collision(Floor* floor);
    bool collision(Player* player);
    
    void setDir(int dx, int dy);
    void setDirX(int dx);
    void setDirY(int dy);
    void setPos(int x, int y);

    // Getters
    int getDirX() const { return dirX; }
    int getDirY() const { return dirY; }
    int getSize() const { return size; }
    int getDiameter() const { return diameter; }
    Sprite* getSprite() const { return sprite; }

    // Friend classes for easier refactoring transition
    friend class Scene;
};