#pragma once

#include "../core/action.h"
#include "../core/motion.h"

class Player;

/**
 * PlayerDeadAction - Encapsulates player death animation using Motion/Action system.
 *
 * Uses real-time rotation instead of pre-rendered frames. The sprite rotates
 * continuously (720 degrees per second = 2 full rotations/sec) while following
 * a physics-based trajectory with gravity and wall bounce.
 *
 * Physics:
 * - Thrown with initial velocity (velX, velY)
 * - Gravity accelerates downward
 * - Bounces off right wall with reversed velocity
 * - Falls below screen to complete
 *
 * Visual:
 * - Rotates sprite continuously using sprite.rotation
 * - Uses single frame sprite (p1dead.png or p2dead.png)
 *
 * Usage:
 *   auto deathAction = std::make_unique<PlayerDeadAction>(player, 5.0f, -4.0f);
 *   deathAction->start();
 *   while (deathAction->update(dt)) { ... }
 */
class PlayerDeadAction : public Action
{
private:
    Player* player;
    float velocityX;
    float velocityY;
    const float GRAVITY = 0.5f;

    // Rotation animation (infinite loop, linear, 360 degrees in 0.5 seconds)
    Motion rotationMotion;

public:
    /**
     * Constructor
     * @param player Pointer to the player
     * @param velX Initial horizontal velocity (pixels per frame)
     * @param velY Initial vertical velocity (negative = up, pixels per frame)
     */
    PlayerDeadAction(Player* player, float velX, float velY);

    void start() override;
    bool update(float dt) override;
};
