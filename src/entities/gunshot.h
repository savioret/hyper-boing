#pragma once

#include "shot.h"

class SpriteSheet;

/**
 * GunShot - Animated bullet projectile
 *
 * Fires animated bullets using a sprite sheet. Has two animation states:
 * - FLIGHT: Normal upward movement with frames 0→1→2→3→4, then loops 3↔4
 * - IMPACT: Plays impact animation (frames 5→6) when hitting non-breakable objects
 *
 * Unlike harpoon, gun bullets don't have a tail - just the animated bullet sprite.
 */
class GunShot : public Shot
{
private:
    SpriteSheet* spriteSheet;  // Non-owning pointer to scene's sprite sheet
    int currentFrame;
    int frameCounter;
    int frameDelay;

    enum class AnimState
    {
        FLIGHT,    // Normal flight animation
        IMPACT     // Impact animation (floor/ceiling hit)
    };
    AnimState animState;

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
    void move() override;
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
     * Advance to next animation frame based on current state
     */
    void advanceFrame();

    /**
     * Trigger impact animation sequence
     */
    void triggerImpact();
};
