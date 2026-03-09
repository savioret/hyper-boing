#pragma once

#include "platform.h"
#include "../core/collisionbox.h"

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
public:
    enum class Color { Red = 0, Blue = 1, Green = 2, Yellow = 3 };

private:
    Scene* scene;
    FloorType type;
    Color color = Color::Red;
    int sx, sy;

public:
    Floor(Scene* scene, int x, int y, FloorType type, int color = 0);
    ~Floor();

    void update(float dt) override;

    int getWidth()  const override { return sx; }
    int getHeight() const override { return sy; }

    Sprite* getCurrentSprite() const override;

    CollisionBox getCollisionBox() const override {
        return { (int)xPos, (int)yPos, sx, sy };
    }
};
