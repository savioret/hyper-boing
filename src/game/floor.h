#pragma once

#include "platform.h"
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

class Floor : public Platform
{
private:
    Scene* scene;
    SingleSprite sprite;

    int sx, sy;
    int id;

public:
    Floor(Scene* scene, int x, int y, int id);
    ~Floor();

    void update(float dt) override;

    int getWidth() const override { return sx; }
    int getHeight() const override { return sy; }
    int getId() const { return id; }

    Sprite* getCurrentSprite() const override;

    CollisionBox getCollisionBox() const override {
        return { (int)xPos, (int)yPos, sx, sy };
    }
};
