#pragma once

#include "gameobject.h"
#include "../game/weapontype.h"

class Scene;
class Player;
class Sprite;
class Floor;

/**
 * Shoot class
 *
 * Represents the projectile object fired by players.
 * Manages its movement, animation, and collision detection logic.
 */
class Shoot : public IGameObject
{
private:
    Scene* scene;
    Player* player; // player who shot it
    Sprite* sprites[3];
    float xPos, yPos;
    float xInit, yInit; // initial x and y

    int sx, sy;

    int id;
    WeaponType weaponType;
    int weaponSpeed;  // Store speed from config
    int frame;

    int tail;  // defines the tail animation
    int tailTime;
    int shotCounter;

public:
    Shoot(Scene* scene, Player* player, WeaponType type, int xOffset = 0);
    ~Shoot();

    WeaponType getWeaponType() const { return weaponType; }

    void move();
    bool collision(Floor* floor);
    
    // Getters for Scene and other classes
    float getX() const { return xPos; }
    float getY() const { return yPos; }
    float getYInit() const { return yInit; }
    Player* getPlayer() const { return player; }
    Sprite* getSprite(int index) const { return sprites[index]; }
    int getTail() const { return tail; }

    // Friend classes for easier refactoring transition
    friend class Scene;
    friend class Ball;
};