#pragma once

#include "shot.h"
#include "../core/animspritesheet.h"
#include <memory>

/**
 * ClawShot - Grappling claw weapon projectile
 *
 * Fires a claw upward that sticks to the ceiling or a platform for 5 seconds.
 * While stuck it can still destroy balls on contact. Disappears after the timer
 * expires or when a ball hits it.
 *
 * Frame layout in claw_weapon.json:
 *   Frame 0 (22x47, xOff=0): claw head  - shown when stuck to surface
 *   Frame 1 ( 4x47, xOff=9): chain tile - tiled vertically in both phases
 *   Frame 2 (16x47, xOff=3): flying tip - shown while travelling upward
 *
 * claw_weapon_yellow.json (2 frames, same layout as frames 0-1 above):
 *   Shown during the last second before the stuck claw disappears.
 */
class ClawShot : public Shot
{
private:
    std::unique_ptr<AnimSpriteSheet> clawAnim;       // Default skin clone
    std::unique_ptr<AnimSpriteSheet> clawYellowAnim; // Warning skin clone (last second)

    bool stuck;
    float stuckTimer;   // Seconds remaining while stuck

    static constexpr float STUCK_DURATION  = 3.5f;
    static constexpr float WARN_THRESHOLD  = 1.0f;  // Switch to yellow skin below this time

public:
    ClawShot(Scene* scn, Player* pl, WeaponType type);
    ~ClawShot() = default;

    void update(float dt) override;
    void draw(Graph* graph) override;
    CollisionBox getCollisionBox() const override;

    /**
     * @brief Get collision box for floor/ceiling collision only
     * @return Collision box from tip down to gun position (head level)
     *
     * This prevents the chain from colliding with platforms that the
     * player's head is above but feet are below (e.g., climbing through).
     */
    CollisionBox getFloorCollisionBox() const override;

    // Collision hooks
    void onBallHit(Ball* b) override;       // looseShoot() + kill()
    void onFloorHit(Platform* f) override;  // Stick to platform bottom
    void onCeilingHit() override;           // Stick to ceiling
};
