#pragma once

#include "platform.h"

/**
 * @enum GlassType
 *
 * Identifies the shape variant of a Glass platform.
 * The enum value is the index of the first (undamaged) frame for that
 * type in the glass_bricks spritesheet, so damage state can be computed
 * as: frame = static_cast<int>(type) + damageLevel
 */
enum class GlassType : int
{
    VERT_BIG     = 0,   ///< Vertical, large  (16×64)  — tag "vert_big",    frames 0–4
    VERT_MIDDLE  = 5,   ///< Vertical, medium (16×48)  — tag "vert_middle", frames 5–9
    HORIZ_BIG    = 10,  ///< Horizontal, large  (64×16) — tag "horiz_big",   frames 10–14
    HORIZ_MIDDLE = 15,  ///< Horizontal, medium (48×16) — tag "horiz_middle", frames 15–19
    SMALL        = 20   ///< Square small (16×16)       — tag "small",       frames 20–24
};

/**
 * Glass class
 *
 * A destructible platform that visually degrades across 5 damage states
 * each time it is hit by a weapon shot. After the fifth hit it disappears.
 *
 * Balls bounce off Glass and players can walk on it, exactly like Floor.
 * Unlike Floor, Glass reacts to weapon hits via onHit().
 */
class Glass : public Platform
{
private:
    GlassType type;
    int damageLevel;  ///< Current damage state (0 = pristine, 4 = fully cracked)
    int sx, sy;       ///< Collision-box dimensions (fixed to undamaged first frame)

public:
    Glass(int x, int y, GlassType type);
    ~Glass() = default;

    void update(float dt) override {}

    /**
     * @brief Advance damage state by one step.
     * After the fifth hit (damageLevel > 4) the platform is killed.
     */
    void onHit() override;

    int getWidth()  const override { return sx; }
    int getHeight() const override { return sy; }

    Sprite* getCurrentSprite() const override;

    CollisionBox getCollisionBox() const override
    {
        return { (int)xPos, (int)yPos, sx, sy };
    }
};
