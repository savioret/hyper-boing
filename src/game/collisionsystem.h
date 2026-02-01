#pragma once

#include <list>
#include <SDL.h>
#include "contact.h"

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
 * @brief Handles collision detection and physics resolution
 *
 * This system is responsible for:
 * - Detecting overlaps between entities
 * - Resolving physics (bouncing balls off walls)
 * - Reporting what touched what (via ContactList)
 *
 * It does NOT handle:
 * - Scoring
 * - Killing entities
 * - Firing gameplay events
 * - Weapon-specific behaviors
 *
 * Those responsibilities belong to CollisionRules.
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
        bool checkPlayerCollisions;      ///< Whether to check ball-player collisions
    };

    CollisionSystem() = default;

    /**
     * @brief Detect all collisions and resolve physics
     *
     * Runs collision detection for all entity pairs, resolves
     * physics (ball bounces), and returns a list of contacts
     * for CollisionRules to process.
     *
     * @param ctx Collision context with entity lists and flags
     * @return List of contacts detected this frame
     */
    ContactList detectAndResolve(Context& ctx);

private:
    /**
     * @brief Detect ball vs shot collisions
     * Records contacts but does NOT kill balls or add score.
     */
    void detectBallVsShot(Context& ctx, ContactList& contacts);

    /**
     * @brief Detect and resolve ball vs floor collisions
     * Resolves physics immediately (bouncing), records contacts.
     */
    void detectBallVsFloor(Context& ctx, ContactList& contacts);

    /**
     * @brief Detect ball vs player collisions
     * Records contacts but does NOT kill players.
     */
    void detectBallVsPlayer(Context& ctx, ContactList& contacts);

    /**
     * @brief Detect shot vs floor collisions
     * Records contacts but does NOT call onFloorHit.
     */
    void detectShotVsFloor(Context& ctx, ContactList& contacts);

    /**
     * @brief Resolve ball-floor collision when multiple floors are hit
     */
    void resolveBallFloorCollision(Ball* b, FloorColision* fc, int moved);
};
