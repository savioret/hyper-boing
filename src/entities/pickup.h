#pragma once

#include "../core/gameobject.h"
#include "../core/singlesprite.h"
#include "../core/animspritesheet.h"
#include "pickuptype.h"

// Forward declarations
class Scene;
class Player;
class Graph;
class Sprite;
class Platform;

// PickupType, MAX_DEATH_PICKUPS and DeathPickupEntry are defined in pickuptype.h

/**
 * Pickup class
 *
 * Collectible items that spawn at specific coordinates, fall linearly
 * to the ground, and grant effects when collected by players.
 *
 * Pivot point is bottom-middle (falls until yPos reaches floor/MAX_Y).
 */
class Pickup : public IGameObject
{
public:
    static constexpr float FALL_SPEED = 2.0f;
    static constexpr int SPRITE_SIZE = 16;
    static constexpr float EXTRA_TIME_BONUS = 20.0f;
    static constexpr float FREEZE_DURATION = 10.0f;
    static constexpr float LIFETIME = 7.0f;        // Seconds before disappearing after landing
    static constexpr float BLINK_DURATION = 3.0f;  // Blink during last 3 seconds

private:
    Scene* scene;
    SingleSprite sprite;
    std::unique_ptr<AnimSpriteSheet> shieldAnim;  // Used only for SHIELD type
    PickupType pickupType;
    bool falling;
    float groundY;  // Y position where pickup stops falling
    float groundTimer;  // Time elapsed since landing (seconds)
    Platform* landedPlatform = nullptr;  // Platform the pickup is resting on (nullptr = ground/MAX_Y)

    /**
     * Find the nearest platform top directly below the pickup's center.
     * Returns Stage::MAX_Y if no platform is found.
     */
    float findGroundBelow() const;

public:
    /**
     * Construct a pickup
     * @param scene Scene context
     * @param x X position (center)
     * @param y Y position (bottom)
     * @param type Type of pickup
     */
    Pickup(Scene* scene, int x, int y, PickupType type);
    ~Pickup() = default;

    /**
     * Update pickup position (fall physics)
     * @param dt Delta time in seconds
     */
    void update(float dt);

    /**
     * Draw the pickup sprite
     * @param graph Graphics context
     */
    void draw(Graph* graph);

    /**
     * Apply effect to player when collected
     * @param player Player who collected the pickup
     */
    void applyEffect(Player* player);

    /**
     * Get collision box for AABB collision detection
     * @return 16x16 collision box centered at position
     */
    CollisionBox getCollisionBox() const override;

    /**
     * Get pickup type
     */
    PickupType getType() const { return pickupType; }

    /**
     * Get pickup type as string for debugging
     */
    static const char* getTypeName(PickupType type);
};
