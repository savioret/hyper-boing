#pragma once

#include <memory>
#include <algorithm>
#include "gameobject.h"
#include "pickuptype.h"

// Forward declaration - only stored as unique_ptr in header
class FrameSequenceAnim;

class Scene;
class Shot;
class Player;
class Platform;
class Sprite;
struct CollisionSide;

/**
 * Hexa class - Anti-gravity hexagonal enemy
 *
 * Different from Ball:
 * - 3 sizes (0-2) instead of 4 (0-3)
 * - Constant velocity movement (no parabolic physics)
 * - Bounces off screen edges AND platforms (reflects direction)
 * - Splash animation on death
 * - Spawns children when popped (same as Ball)
 */
class Hexa : public IGameObject
{
private:
    float velX, velY;           // Constant velocity components
    int size;                   // 0=large(57x45), 1=medium(31x24), 2=small(16x12)
    int width, height;          // Current sprite dimensions
    Scene* scene;
    DeathPickupEntry deathPickups[MAX_DEATH_PICKUPS] = {};
    int deathPickupCount = 0;

    // Per-instance animation controller (cycles through 4 frames for current size)
    std::unique_ptr<FrameSequenceAnim> animCtrl;

    // Hit flash state
    bool flashing = false;
    float flashTimer = 0.0f;
    static constexpr float FLASH_DURATION = 0.04f;  // 40ms

    // Static size dimensions (from hexa_green.json sourceSize/frame bounds)
    static constexpr int SIZES[][2] = {
        {51, 51},  // Size 0: Large
        {28, 27},  // Size 1: Medium
        {17, 16}   // Size 2: Small
    };

public:
    /**
     * Create a new Hexa at position with velocity
     * @param scene Parent scene
     * @param x X position (top-left)
     * @param y Y position (top-left)
     * @param size Size (0=large, 1=medium, 2=small)
     * @param velX Horizontal velocity
     * @param velY Vertical velocity
     */
    Hexa(Scene* scene, int x, int y, int size, float velX, float velY);

    /**
     * Create a child Hexa from a parent
     * @param scene Parent scene
     * @param parent Parent hexa being destroyed
     * @param dirX Horizontal direction (-1=left, 1=right)
     * @param dirY Vertical direction (-1=up, 1=down)
     */
    Hexa(Scene* scene, Hexa* parent, int dirX, int dirY);

    ~Hexa() = default;

    /**
     * Update position with constant velocity
     * @param dt Delta time in seconds (used for screen bounds check only)
     */
    void update(float dt);

    /**
     * Called when hexa dies - spawns splash effect
     */
    void onDeath() override;

    /**
     * Handle bounce when hitting a platform
     * @param side Which side of the platform was hit
     */
    void handlePlatformBounce(const CollisionSide& side);

    /**
     * Create child hexas when destroyed
     * @return Pair of unique_ptr to children (nullptr if size 2)
     */
    std::pair<std::unique_ptr<Hexa>, std::unique_ptr<Hexa>> createChildren();

    // Collision detection methods
    bool collision(Shot* shot);
    bool collision(Platform* platform);
    bool collision(Player* player);

    // Setters
    /// Legacy: add a pickup entry bound to this hexa's current size.
    void setDeathPickup(PickupType type)
    {
        if (deathPickupCount < MAX_DEATH_PICKUPS)
            deathPickups[deathPickupCount++] = { size, type };
    }

    /// Copy all pickup entries from a params array (used when spawning from stage).
    void setDeathPickups(const DeathPickupEntry* entries, int count)
    {
        deathPickupCount = (count < MAX_DEATH_PICKUPS) ? count : MAX_DEATH_PICKUPS;
        for (int i = 0; i < deathPickupCount; i++)
            deathPickups[i] = entries[i];
    }

    /// Add a single sized pickup entry (used when propagating from a parent hexa).
    void addDeathPickup(const DeathPickupEntry& entry)
    {
        if (deathPickupCount < MAX_DEATH_PICKUPS)
            deathPickups[deathPickupCount++] = entry;
    }

    void setVelX(float vx) { velX = vx; }
    void setVelY(float vy) { velY = vy; }

    // Getters
    int getSize() const { return size; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getVelX() const { return velX; }
    float getVelY() const { return velY; }
    bool isInFlashState() const { return flashing; }
    Sprite* getCurrentSprite() const;

    int getCollisionRadius() const { return std::min(width, height) / 2; }
    void getCollisionCenter(int& cx, int& cy) const {
        cx = (int)xPos + width / 2;
        cy = (int)yPos + height / 2;
    }

    /**
     * Kill the hexa - starts flash effect before actual death
     */
    void kill() override;

    /**
     * Get AABB for broad-phase / fallback; circle methods are used for actual detection
     */
    CollisionBox getCollisionBox() const override {
        return { (int)xPos, (int)yPos, width, height };
    }
};
