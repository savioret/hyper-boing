#pragma once

#include "gameobject.h"
#include "../game/weapontype.h"
#include "../game/collisionsystem.h"

// Forward declarations
class Scene;
class Player;
class Ball;
class Platform;
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
    float gunY;  // Y position of gun (head level) - used for floor collision

    WeaponType weaponType;
    int weaponSpeed;
    int audioChannel;  // SDL_mixer channel for weapon sound (-1 if none)

public:
    /**
     * Constructor
     * @param scn Scene this shot belongs to
     * @param pl Player who fired the shot
     * @param type Weapon type for this shot
     */
    Shot(Scene* scn, Player* pl, WeaponType type);
    virtual ~Shot() {}

    // IGameObject lifecycle hook
    void onDeath() override;

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
     * Called when shot hits a platform (Floor or Glass)
     * Default: Decrements player shot count and kills the shot
     */
    virtual void onFloorHit(Platform* f);

    /**
     * Called when shot hits the ceiling (Y <= MIN_Y)
     * Default: Decrements player shot count and kills the shot
     */
    virtual void onCeilingHit();

    // Accessors
    float getXInit() const { return xInit; }
    float getYInit() const { return yInit; }
    float getGunY() const { return gunY; }  // Gun/head position for floor collision

    Player* getPlayer() const { return player; }
    Scene* getScene() const { return scene; }
    WeaponType getWeaponType() const { return weaponType; }
    int getSpeed() const { return weaponSpeed; }
    
    // Audio channel management
    void setAudioChannel(int channel) { audioChannel = channel; }
    int getAudioChannel() const { return audioChannel; }

    // Collision detection
    /**
     * @brief Get collision box for AABB collision detection
     * @return Collision box in top-left coordinate space
     *
     * Pure virtual - each weapon type defines its own collision bounds.
     * Harpoon returns the chain area, gun returns the bullet sprite bounds.
     * Used for ball/hexa collision - covers the entire chain.
     */
    CollisionBox getCollisionBox() const override = 0;

    /**
     * @brief Get collision box specifically for floor/ceiling collision
     * @return Collision box from tip down to gun position (not full chain)
     *
     * For chain weapons (harpoon/claw), returns only the portion above the
     * gun position. This prevents false collisions when player's head is
     * above a platform but feet are below (e.g., climbing through a platform).
     *
     * Default implementation returns same as getCollisionBox().
     * Override in chain weapons to return the gun-to-tip portion only.
     */
    virtual CollisionBox getFloorCollisionBox() const { return getCollisionBox(); }

    /**
     * Check collision with floor
     * @param fl Floor to check collision with
     * @return true if colliding, false otherwise
     *
     * Virtual to allow weapon-specific collision logic (e.g., gun vs harpoon)
     */
    virtual bool collision(Platform* fl);
};
