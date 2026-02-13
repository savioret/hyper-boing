#pragma once

#include "gameobject.h"
#include "singlesprite.h"
#include "collisionsystem.h"

/**
 * Floor class
 *
 * It is the "floor" object, the platforms or blocks
 * that are floating in the air, which only
 * disturb the trajectory of the Balls.
 */
class Scene;

class Floor : public IGameObject
{
private:
    Scene* scene;
    SingleSprite sprite;

    int sx, sy;
    int id;

public:
    Floor(Scene* scene, int x, int y, int id);
    ~Floor();

    void update(float dt);

    int getWidth() const { return sx; }
    int getHeight() const { return sy; }
    int getId() const { return id; }

    /**
     * @brief Get collision box for AABB collision detection
     * @return Collision box in top-left coordinate space
     */
    CollisionBox getCollisionBox() const override {
        return { (int)xPos, (int)yPos, sx, sy };
    }
};