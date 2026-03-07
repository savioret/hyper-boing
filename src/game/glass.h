#pragma once

#include "platform.h"
#include "../entities/pickup.h"
#include "../core/animcontroller.h"
#include <memory>

class Scene;

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
 * A destructible platform that breaks on a single hit. When hit, a destruction
 * animation plays through 5 frames and the platform is removed when complete.
 *
 * Balls bounce off Glass and players can walk on it, exactly like Floor.
 * Unlike Floor, Glass reacts to weapon hits via onHit().
 */
class Glass : public Platform
{
private:
    GlassType type;
    int sx, sy;       ///< Collision-box dimensions (fixed to undamaged first frame)
    Scene* scene = nullptr;
    bool hasDeathPickup = false;
    PickupType deathPickupType = PickupType::GUN;

    bool breaking = false;  ///< True when destruction animation is playing
    std::unique_ptr<FrameSequenceAnim> breakAnim;  ///< Destruction animation

public:
    Glass(Scene* scene, int x, int y, GlassType type);
    ~Glass() = default;

    void update(float dt) override;

    void setDeathPickup(PickupType type) { hasDeathPickup = true; deathPickupType = type; }

    /**
     * @brief Start destruction animation on first hit.
     * The glass will be killed when the animation completes.
     */
    void onHit() override;
    void onDeath() override;

    int getWidth()  const override { return sx; }
    int getHeight() const override { return sy; }

    bool isDestructible() const override { return true; }

    Sprite* getCurrentSprite() const override;

    CollisionBox getCollisionBox() const override
    {
        return { (int)xPos, (int)yPos, sx, sy };
    }
};
