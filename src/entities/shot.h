#pragma once

#include "gameobject.h"
#include "../game/weapontype.h"

// Forward declarations
class Scene;
class Player;
class Ball;
class Floor;
class Graph;

/**
 * Shot - Abstract base class for all weapon projectiles
 *
 * Provides common functionality for projectiles while allowing weapon-specific
 * behavior through virtual methods. Inherits from IGameObject for automatic
 * garbage collection via the deferred deletion pattern.
 *
 * Subclasses must implement:
 *   - move(): Update projectile position and state
 *   - draw(): Render the projectile
 *
 * Subclasses may override:
 *   - onBallHit(): Custom behavior when hitting a ball
 *   - onFloorHit(): Custom behavior when hitting a floor
 *   - onCeilingHit(): Custom behavior when hitting ceiling
 */
class Shot : public IGameObject
{
protected:
    Scene* scene;
    Player* player;
    float xInit, yInit;

    WeaponType weaponType;
    int weaponSpeed;

public:
    /**
     * Constructor
     * @param scn Scene this shot belongs to
     * @param pl Player who fired the shot
     * @param type Weapon type for this shot
     * @param xOffset Horizontal offset for multi-projectile weapons
     */
    Shot(Scene* scn, Player* pl, WeaponType type, int xOffset);
    virtual ~Shot() {}

    // Abstract interface - must be implemented by subclasses
    virtual void update(float dt) = 0;
    virtual void draw(Graph* graph) = 0;

    // Collision hooks - can be overridden by subclasses
    /**
     * Called when shot hits a ball
     * Default: Just kills the shot
     */
    virtual void onBallHit(Ball* b);

    /**
     * Called when shot hits a floor
     * Default: Decrements player shot count and kills the shot
     */
    virtual void onFloorHit(Floor* f);

    /**
     * Called when shot hits the ceiling (Y <= MIN_Y)
     * Default: Decrements player shot count and kills the shot
     */
    virtual void onCeilingHit();

    // Accessors
    float getXInit() const { return xInit; }
    float getYInit() const { return yInit; }

    Player* getPlayer() const { return player; }
    Scene* getScene() const { return scene; }
    WeaponType getWeaponType() const { return weaponType; }
    int getSpeed() const { return weaponSpeed; }

    // Collision detection helper
    /**
     * Check collision with floor
     * @param fl Floor to check collision with
     * @return true if colliding, false otherwise
     *
     * Virtual to allow weapon-specific collision logic (e.g., gun vs harpoon)
     */
    virtual bool collision(Floor* fl);
};
