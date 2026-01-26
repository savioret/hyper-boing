#pragma once

#include "../game/weapontype.h"
#include "eventmanager.h"
#include <memory>
#include "../core/gameobject.h"
#include "../core/sprite2d.h"
#include "../core/animcontroller.h"

class Scene;
class Action;

/**
 * Player facing direction
 */
enum class FacingDirection
{
    RIGHT,  // Player is facing right
    LEFT    // Player is facing left
};

/**
 * Player class
 *
 * Player, as its name suggests, contains information about the sprite
 * identifying the player, movement and action keys, score, lives,
 * weapons, and internal management data.
 *
 * Inherits from both IGameObject (for lifecycle management) and Sprite2D
 * (for rendering). Position is delegated to Sprite2D as the single source of truth.
 */
class Player : public IGameObject, public Sprite2D
{
private:
    int xDir, yDir; // used for death animation (legacy, may be removed)
    FacingDirection facing;  // Direction player is facing
    int lives;
    int score;
    int id;  // 0 = player 1, 1 = player 2
    int idWeapon;  // Kept for backward compatibility, not used
    WeaponType currentWeapon;
    int maxShoots;
    int numShoots;
    int shotCounter;
    int shotInterval; // time between shots
    int animSpeed;
    int animCounter;
    int moveIncrement; // displacement increment when walking
    bool playing;
    int immuneCounter; // for when just revived

    // Animation controller for state-based animations
    std::unique_ptr<StateMachineAnim> animController;

    // Death animation using Action system
    std::unique_ptr<Action> deathAction;
    EventManager::ListenerHandle playerDiedHandle;

public:
    Player(int id);
    ~Player();

    // IGameObject position delegation to Sprite2D (single source of truth)
    void setPos(float x, float y) override { Sprite2D::setPos(x, y); }
    void setX(float x) override { Sprite2D::setX(x); }
    void setY(float y) override { Sprite2D::setY(y); }
    float getX() const override { return Sprite2D::getX(); }
    float getY() const override { return Sprite2D::getY(); }

    // IGameObject lifecycle (isDead() is inherited, kill() has Player-specific logic)
    void kill() override;
    void onDeath() override;

    void init();
    // void setFrame(int frame); // Inherited from Sprite2D
    void update(float dt);
    void addScore(int num);
    void moveLeft();
    void moveRight();
    void stop();

    bool canShoot() const;
    void shoot();
    void looseShoot();
    void revive();

    // Event handlers
    void onPlayerHit();

    // Weapon management
    WeaponType getWeapon() const { return currentWeapon; }
    void setWeapon(WeaponType type);

    // Getters
    // getX, getY are overridden (delegate to Sprite2D)
    // getWidth, getHeight, getFrame are inherited from Sprite2D
    // isDead is inherited from IGameObject
    int getId() const { return id; }
    int getScore() const { return score; }
    int getLives() const { return lives; }
    bool isPlaying() const { return playing; }
    bool isImmune() const { return immuneCounter > 0; }
    int getNumShoots() const { return numShoots; }
    int getIdWeapon() const { return idWeapon; }
    Sprite* getSprite() const { return getCurrentSprite(); } // Compatibility wrapper
    FacingDirection getFacing() const { return facing; }

    // Setters
    // setX, setY are inherited
    void setPlaying(bool p) { playing = p; }
    
    // For access during refactoring
    friend class Scene;
    friend class Shoot;
    friend class Ball;
};