#pragma once

#include <list>
#include <SDL.h>

// Forward declarations
class Ball;
class Shot;
class Floor;
class Player;

/**
 * @class FloorColision
 * @brief Auxiliary class for handling collisions between balls and floors
 *
 * Stores collision information when a ball intersects with a floor,
 * including which floor was hit and at what point.
 */
class FloorColision
{
public:
    Floor* floor;      ///< Pointer to the floor involved in collision
    SDL_Point point;   ///< Collision point coordinates

    FloorColision() : floor(nullptr) { point.x = point.y = 0; }
};

/**
 * @class CollisionSystem
 * @brief Handles all collision detection and response for the game
 *
 * Separates collision logic from Scene, making it easier to test and maintain.
 * Checks four collision types:
 * - Ball vs Shot (destroy ball, add score)
 * - Ball vs Floor (bounce with direction reversal)
 * - Ball vs Player (kill player)
 * - Shot vs Floor (destroy shot)
 */
class CollisionSystem
{
public:
    /**
     * @struct Context
     * @brief Contains all data needed for collision detection
     */
    struct Context
    {
        std::list<Ball*>& balls;         ///< Active balls
        std::list<Shot*>& shots;         ///< Active shots
        std::list<Floor*>& floors;       ///< Active floors
        Player* players[2];              ///< Players (may be nullptr)
        bool checkPlayerCollisions;      ///< Whether to check ball-player collisions (false during GameOver)
    };

    CollisionSystem() = default;

    /**
     * @brief Run all collision checks for this frame
     * @param ctx Collision context with entity lists and flags
     */
    void checkAll(Context& ctx);

private:
    /**
     * @brief Check ball vs shot collisions
     * When a ball is hit by a shot:
     * - Call shot->onBallHit(ball)
     * - Add score to shooting player
     * - Fire BALL_HIT event
     * - Kill ball
     */
    void checkBallVsShot(Context& ctx);

    /**
     * @brief Check ball vs floor collisions
     * When a ball hits a floor:
     * - Reverse ball direction based on collision side
     * - Handle complex multi-floor corner cases
     */
    void checkBallVsFloor(Context& ctx);

    /**
     * @brief Check ball vs player collisions
     * When a ball hits a player:
     * - Fire PLAYER_HIT event
     * - Kill player (unless immune)
     */
    void checkBallVsPlayer(Context& ctx);

    /**
     * @brief Check shot vs floor collisions
     * When a shot hits a floor:
     * - Call shot->onFloorHit(floor)
     */
    void checkShotVsFloor(Context& ctx);

    /**
     * @brief Resolve ball-floor collision when multiple floors are hit
     *
     * Complex logic for handling corner cases where a ball hits
     * two floors simultaneously (e.g., corner of an L-shape).
     *
     * @param b Ball that collided
     * @param fc Array of floor collisions (size 2)
     * @param moved Which direction was already processed (0=none, 1=X, 2=Y)
     */
    void resolveBallFloorCollision(Ball* b, FloorColision* fc, int moved);

    /**
     * @brief Calculate score for destroying an object
     * @param diameter Object diameter (ball size indicator)
     * @return Score value (1000 / diameter)
     */
    static int objectScore(int diameter);
};
