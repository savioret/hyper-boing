#pragma once

#include <SDL.h>
#include <memory>
#include "gameobject.h"
#include "../game/collisionsystem.h"
#include "pickup.h"

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

    int dirX, dirY; // direction
    float time, maxTime;
    float y0; // initial space
    float gravity; // acceleration (acc)
    Scene* scene;
    bool hasDeathPickup = false;
    PickupType deathPickupType = PickupType::GUN;

    // Hit flash state (like Hexa)
    bool flashing = false;
    float flashTimer = 0.0f;
    static constexpr float FLASH_DURATION = 0.04f;  // 40ms

public:
    Ball(Scene* scene, Ball* oldBall, int dir);
    Ball(Scene* scene, int x, int y, int size, int dirX = 1, int dirY = 1, int top = 0, int id = 0);
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

    void setDir(int dx, int dy);
    void setDirX(int dx);
    void setDirY(int dy);
    void setPos(int x, int y);
    void setDeathPickup(PickupType type) { hasDeathPickup = true; deathPickupType = type; }

    // Getters
    int getDirX() const { return dirX; }
    int getDirY() const { return dirY; }
    int getSize() const { return size; }
    float getTime() const { return time; }
    int getDiameter() const { return diameter; }
    bool isInFlashState() const { return flashing; }
    Sprite* getCurrentSprite() const;

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