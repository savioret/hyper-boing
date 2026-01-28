#pragma once

#include "shot.h"
#include "../core/animcontroller.h"
#include <memory>

class SpriteSheet;

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
    SpriteSheet* spriteSheet;  // Non-owning pointer to scene's sprite sheet
    std::unique_ptr<StateMachineAnim> animController;
    bool inImpact = false;     // Track if we're in impact state (for movement logic)

public:
    /**
     * Constructor
     * @param scn Scene this shot belongs to
     * @param pl Player who fired the shot
     * @param sheet Sprite sheet containing gun bullet frames
     * @param xOffset Horizontal offset for multi-projectile weapons
     */
    GunShot(Scene* scn, Player* pl, SpriteSheet* sheet, int xOffset);
    ~GunShot();

    // Implement abstract interface from Shot
    void update(float dt) override;
    void draw(Graph* graph) override;

    // Override collision hooks for impact animation
    void onFloorHit(Floor* f) override;
    void onCeilingHit() override;
    void onBallHit(Ball* b) override;

    // Override collision detection to match bullet sprite center
    bool collision(Floor* fl) override;

    // Accessors for debug visualization
    Sprite* getCurrentSprite() const;

private:
    /**
     * Trigger impact animation sequence
     */
    void triggerImpact();
};
