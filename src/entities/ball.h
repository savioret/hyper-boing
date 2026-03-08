#pragma once

#include <SDL.h>
#include <memory>
#include "gameobject.h"
#include "../core/collisionbox.h"
#include "pickuptype.h"

class Scene;
class Shot;
class Player;
class Platform;
class Sprite;

/**
 * Ball class
 *
 * It is the game ball, which must be shot.
 */
class Ball : public IGameObject
{
private:
    int top; // maximum height from the floor
    int diameter;
    int size;
    int id;

    float dirX; // horizontal direction (float: 1.0=right, -1.0=left; magnitude acts as speed multiplier)
    int dirY;   // vertical direction (-1=up, 1=down)
    float time, maxTime;
    float y0; // initial space
    float gravity; // acceleration (acc)
    Scene* scene;
    DeathPickupEntry deathPickups[MAX_DEATH_PICKUPS] = {};
    int deathPickupCount = 0;

    // Hit flash state (like Hexa)
    bool flashing = false;
    float flashTimer = 0.0f;
    static constexpr float FLASH_DURATION = 0.04f;  // 40ms

public:
    Ball(Scene* scene, Ball* oldBall, int dir);
    Ball(Scene* scene, int x, int y, int size, float dirX = 1.0f, int dirY = 1, int top = 0, int id = 0);
    ~Ball();

    void init();
    void initTop();

    void update(float dt);

    // IGameObject lifecycle hook
    void onDeath() override;

    /**
     * @brief Create child balls when this ball is destroyed
     *
     * If ball is large enough (size < 3), creates two smaller balls.
     * Returns a pair of unique_ptr, which will be nullptr if the ball
     * is too small to split.
     *
     * @return Pair of unique_ptr to child balls (nullptr if no split)
     */
    std::pair<std::unique_ptr<Ball>, std::unique_ptr<Ball>> createChildren();

    bool collision(Shot* shot);
    bool collision(Platform* floor);
    bool collision(Player* player);

    void setDir(float dx, int dy);
    void setDirX(float dx);
    void setDirY(int dy);
    void setPos(int x, int y);

    /// Legacy: add a pickup entry bound to this ball's current size.
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

    /// Add a single sized pickup entry (used when propagating from a parent ball).
    void addDeathPickup(const DeathPickupEntry& entry)
    {
        if (deathPickupCount < MAX_DEATH_PICKUPS)
            deathPickups[deathPickupCount++] = entry;
    }

    // Getters
    float getDirX() const { return dirX; }
    int getDirY() const { return dirY; }
    int getSize() const { return size; }
    float getTime() const { return time; }
    int getDiameter() const { return diameter; }
    bool isInFlashState() const { return flashing; }
    Sprite* getCurrentSprite() const;

    /**
     * @brief Get collision radius for circle collision detection
     * @return Half the ball's diameter
     */
    int getCollisionRadius() const { return diameter / 2; }

    /**
     * @brief Get collision center point for circle collision detection
     * @param cx Output x coordinate of center
     * @param cy Output y coordinate of center
     */
    void getCollisionCenter(int& cx, int& cy) const {
        cx = (int)xPos + diameter / 2;
        cy = (int)yPos + diameter / 2;
    }

    /**
     * Kill the ball - starts flash effect before actual death
     */
    void kill() override;

    /**
     * @brief Get collision box for AABB collision detection
     * @return Collision box in top-left coordinate space
     */
    CollisionBox getCollisionBox() const override {
        return { (int)xPos, (int)yPos, diameter, diameter };
    }
};