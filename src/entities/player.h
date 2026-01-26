#pragma once

#include "../game/weapontype.h"
#include "eventmanager.h"
#include <memory>
#include "../core/sprite2d.h"

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
 */
class Player : public Sprite2D
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
    bool dead;
    bool playing;
    int immuneCounter; // for when just revived

    // Death animation using Action system
    std::unique_ptr<Action> deathAction;
    EventManager::ListenerHandle playerDiedHandle;

public:
    Player(int id);
    ~Player();

    void init();
    // void setFrame(int frame); // Inherited from Sprite2D
    void update(float dt);
    void addScore(int num);
    void moveLeft();
    void moveRight();
    void stop();

    bool canShoot() const;
    bool isDead() const { return dead; }
    void shoot();
    void looseShoot();
    void kill();
    void revive();

    // Event handlers
    void onPlayerHit();

    // Weapon management
    WeaponType getWeapon() const { return currentWeapon; }
    void setWeapon(WeaponType type);

    // Getters
    // getX, getY, getWidth, getHeight, getFrame are inherited from Sprite2D
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