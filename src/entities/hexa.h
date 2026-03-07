#pragma once

#include <memory>
#include "gameobject.h"
#include "pickup.h"

class Scene;
class Shot;
class Player;
class Platform;
class AnimSpriteSheet;
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
    bool hasDeathPickup = false;
    PickupType deathPickupType = PickupType::GUN;

    // Rotation state (discrete steps at ~60ms intervals)
    float rotation = 0.0f;              // Current rotation angle (degrees)
    float rotationTimer = 0.0f;         // Time accumulator for rotation steps
    static constexpr float ROTATION_INTERVAL = 0.06f;  // 60ms between rotation steps
    static constexpr float ROTATION_STEP = 15.0f;      // Degrees per step

    // Hit flash state
    bool flashing = false;
    float flashTimer = 0.0f;
    static constexpr float FLASH_DURATION = 0.04f;  // 40ms

    // Static size dimensions (from JSON)
    static constexpr int SIZES[][2] = {
        {57, 45},  // Size 0: Large
        {31, 24},  // Size 1: Medium
        {16, 12}   // Size 2: Small
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
     * @param dir Direction (-1=left, 1=right)
     */
    Hexa(Scene* scene, Hexa* parent, int dir);

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
    void setDeathPickup(PickupType type) { hasDeathPickup = true; deathPickupType = type; }
    void setVelX(float vx) { velX = vx; }
    void setVelY(float vy) { velY = vy; }

    // Getters
    int getSize() const { return size; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float getVelX() const { return velX; }
    float getVelY() const { return velY; }
    float getRotation() const { return rotation; }
    bool isInFlashState() const { return flashing; }
    Sprite* getCurrentSprite() const;

    /**
     * Kill the hexa - starts flash effect before actual death
     */
    void kill() override;

    /**
     * Get collision box for AABB collision detection
     * @return Collision box in top-left coordinate space
     */
    CollisionBox getCollisionBox() const override {
        return { (int)xPos, (int)yPos, width, height };
    }
};
