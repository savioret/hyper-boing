#pragma once

#include "gameobject.h"

/**
 * Floor class
 *
 * It is the "floor" object, the platforms or blocks
 * that are floating in the air, which only
 * disturb the trajectory of the Balls.
 */
class Scene;
class Sprite;

class Floor : public IGameObject
{
private:
    Scene* scene;
    Sprite* sprite;

    int x, y;
    int sx, sy;
    int id;

public:
    Floor(Scene* scene, int x, int y, int id);
    ~Floor();

    void update();
    
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return sx; }
    int getHeight() const { return sy; }
    int getId() const { return id; }
};