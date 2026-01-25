#pragma once

#include "../game/weapontype.h"
#include "eventmanager.h"
#include <memory>

class Scene;
class Sprite;
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
class Player
{
private:
    Sprite* sprite;
    float xPos, yPos;
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
    int shotInterval; // time between shots // TODO: this should now be part of the Weapon properties
    int animSpeed;
    int animCounter;
    int moveIncrement; // displacement increment when walking
    int frame;
    bool dead;
    bool playing;
    int immuneCounter; // for when just revived
    bool visible;

    // Death animation using Action system
    std::unique_ptr<Action> deathAction;
    EventManager::ListenerHandle playerDiedHandle;

public:
    Player(int id);
    ~Player();

    void init();
    void setFrame(int frame);
    void update(float dt);  // Now takes delta time for action updates
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
    float getX() const { return xPos; }
    float getY() const { return yPos; }
    int getWidth() const { return sprite->getWidth(); }
    int getHeight() const { return sprite->getHeight(); }
    int getId() const { return id; }
    int getScore() const { return score; }
    int getLives() const { return lives; }
    bool isPlaying() const { return playing; }
    bool isVisible() const { return visible; }
    bool isImmune() const { return immuneCounter > 0; }
    int getNumShoots() const { return numShoots; }
    int getIdWeapon() const { return idWeapon; }
    int getFrame() const { return frame; }
    Sprite* getSprite() const { return sprite; }
    FacingDirection getFacing() const { return facing; }

    // Setters
    void setX(float x) { xPos = x; }
    void setY(float y) { yPos = y; }
    void setPlaying(bool p) { playing = p; }
    void setVisible(bool v) { visible = v; }
    
    // For access during refactoring
    friend class Scene;
    friend class Shoot;
    friend class Ball;
};