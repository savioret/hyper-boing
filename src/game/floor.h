#pragma once

#include "platform.h"
#include "collisionsystem.h"

/**
 * @enum FloorType
 *
 * Identifies the shape variant of a Floor platform.
 * The enum value equals the frame index in the floor_bricks spritesheet.
 */
enum class FloorType : int
{
    HORIZ_BIG    = 0,   ///< Horizontal, large  — frame 2
    HORIZ_MIDDLE = 1,   ///< Horizontal, medium — frame 3
    VERT_BIG     = 2,   ///< Vertical, large    — frame 0
    VERT_MIDDLE  = 3,   ///< Vertical, medium   — frame 1
    SMALL        = 4    ///< Square small       — frame 4
};

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
    FloorType type;
    int sx, sy;

public:
    Floor(Scene* scene, int x, int y, FloorType type);
    ~Floor();

    void update(float dt) override;

    int getWidth()  const override { return sx; }
    int getHeight() const override { return sy; }

    Sprite* getCurrentSprite() const override;

    CollisionBox getCollisionBox() const override {
        return { (int)xPos, (int)yPos, sx, sy };
    }
};
