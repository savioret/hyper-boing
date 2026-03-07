#pragma once

#include "shot.h"  
#include "../core/animspritesheet.h"

class AnimSpriteSheet;

/**
 * GunShot - Animated bullet projectile
 *
 * Fires animated bullets using a sprite sheet. Uses StateMachineAnim for animation:
 * - flight_intro: Startup sequence (frames 0→1→2→3→4)
 * - flight_loop: Looping flight (frames 3↔4)
 * - impact: Impact animation (frames 5→6) when hitting non-breakable objects
 *
 * Unlike harpoon, gun bullets don't have a tail - just the animated bullet sprite.
 */
class GunShot : public Shot
{
private:
    std::unique_ptr<AnimSpriteSheet> anim;
    bool inImpact = false;     // Track if we're in impact state (for movement logic)

public:
    /**
     * Constructor
     * @param scn Scene this shot belongs to
     * @param pl Player who fired the shot
     * @param animSheet AnimSpriteSheet containing gun bullet frames and animation
     */
    GunShot(Scene* scn, Player* pl, AnimSpriteSheet* animSheet);
    ~GunShot();

    // Implement abstract interface from Shot
    void update(float dt) override;
    void draw(Graph* graph) override;

    /**
     * @brief Get collision box for the gun bullet sprite
     * @return Collision box matching the current animated sprite frame
     */
    CollisionBox getCollisionBox() const override;

    // Override collision hooks for impact animation
    void onFloorHit(Platform* f) override;
    void onCeilingHit() override;
    void onBallHit(Ball* b) override;

    // Override collision detection to match bullet sprite center
    bool collision(Platform* fl) override;

    // Accessors for debug visualization
    Sprite* getCurrentSprite() const { return anim->getCurrentSprite(); }

private:
    /**
     * Trigger impact animation sequence
     */
    void triggerImpact(float impactY);
};
