#pragma once

#include <list>
#include <memory>
#include <vector>
#include <SDL.h>
#include <algorithm>
#include "../core/collisionbox.h"
#include "contact.h"

// Forward declarations
class Ball;
class Shot;
class Floor;
class Player;
class Pickup;

/**
 * @brief Compute which side of target was hit by mover using minimum penetration depth
 * @param mover The moving collision box
 * @param target The target collision box being hit
 * @return CollisionSide indicating which side(s) were hit
 */
inline CollisionSide getCollisionSide(const CollisionBox& mover, const CollisionBox& target)
{
    CollisionSide side;

    // Calculate penetration depth from each direction
    int penetrationLeft = (mover.x + mover.w) - target.x;      // Mover hitting left side of target
    int penetrationRight = (target.x + target.w) - mover.x;    // Mover hitting right side of target
    int penetrationTop = (mover.y + mover.h) - target.y;       // Mover hitting top side of target
    int penetrationBottom = (target.y + target.h) - mover.y;   // Mover hitting bottom side of target

    // Find minimum penetration on each axis
    int minPenetrationX = std::min(penetrationLeft, penetrationRight);
    int minPenetrationY = std::min(penetrationTop, penetrationBottom);

    // The axis with smaller penetration is the collision axis
    if (minPenetrationX < minPenetrationY) {
        // Horizontal collision
        side.x = (penetrationLeft < penetrationRight) ? SIDE_LEFT : SIDE_RIGHT;
    } else if (minPenetrationY < minPenetrationX) {
        // Vertical collision
        side.y = (penetrationTop < penetrationBottom) ? SIDE_TOP : SIDE_BOTTOM;
    } else {
        // Corner hit - equal penetration on both axes
        side.x = (penetrationLeft < penetrationRight) ? SIDE_LEFT : SIDE_RIGHT;
        side.y = (penetrationTop < penetrationBottom) ? SIDE_TOP : SIDE_BOTTOM;
    }

    return side;
}

/**
 * @brief Get which side of entity A was hit by entity B
 * @param c Contact containing collision information
 * @return CollisionSide for entity A
 */
inline CollisionSide getCollisionSideA(const Contact& c)
{
    return getCollisionSide(c.boxB, c.boxA);
}

/**
 * @brief Get which side of entity B was hit by entity A
 * @param c Contact containing collision information
 * @return CollisionSide for entity B
 */
inline CollisionSide getCollisionSideB(const Contact& c)
{
    return getCollisionSide(c.boxA, c.boxB);
}

/**
 * @brief Check if two collision boxes intersect and compute midpoint (SDL_Point version)
 * @param a First collision box
 * @param b Second collision box
 * @param outMidpoint Output parameter for midpoint of overlap region
 * @return true if boxes overlap, false otherwise
 *
 * @note Prefer using getOverlapCenter() from collisionbox.h for new code.
 *       This overload is provided for SDL_Point convenience.
 */
inline bool intersects(const CollisionBox& a, const CollisionBox& b, SDL_Point& outMidpoint)
{
    return getOverlapCenter(a, b, outMidpoint.x, outMidpoint.y);
}

/**
 * @class CollisionSystem
 * @brief Handles collision detection and physics resolution
 *
 * Architecture:
 * - Phase 1 (Detection): const methods that only build ContactList
 * - Phase 2 (Resolution): processes contacts to modify game state (ball bounces)
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
        std::list<std::unique_ptr<Ball>>& balls;    ///< Active balls
        std::list<std::unique_ptr<Shot>>& shots;    ///< Active shots
        std::list<std::unique_ptr<Floor>>& floors;  ///< Active floors
        std::list<std::unique_ptr<Pickup>>& pickups; ///< Active pickups
        Player* players[2];                         ///< Players (may be nullptr)
        bool checkPlayerCollisions;                 ///< Whether to check ball-player collisions
    };

    CollisionSystem() = default;

    /**
     * @brief Detect all collisions and resolve physics
     *
     * Runs in two phases:
     * 1. Detection: builds ContactList (const operations)
     * 2. Resolution: applies physics (ball bounces)
     *
     * @param ctx Collision context with entity lists and flags
     * @return List of contacts detected this frame
     */
    ContactList detectAndResolve(Context& ctx);

private:
    // ========== Phase 1: Detection (const - only builds contacts) ==========

    /**
     * @brief Detect ball vs shot collisions
     * Records contacts. Does NOT modify any entities.
     */
    void detectBallVsShot(const Context& ctx, ContactList& contacts) const;

    /**
     * @brief Detect ball vs floor collisions
     * Records contacts with collision sides for later physics resolution.
     * Does NOT modify any entities.
     */
    void detectBallVsFloor(const Context& ctx, ContactList& contacts) const;

    /**
     * @brief Detect ball vs player collisions
     * Records contacts. Does NOT modify any entities.
     */
    void detectBallVsPlayer(const Context& ctx, ContactList& contacts) const;

    /**
     * @brief Detect shot vs floor collisions
     * Records contacts. Does NOT modify any entities.
     */
    void detectShotVsFloor(const Context& ctx, ContactList& contacts) const;

    /**
     * @brief Detect pickup vs player collisions
     * Records contacts. Does NOT modify any entities.
     */
    void detectPickupVsPlayer(const Context& ctx, ContactList& contacts) const;

    // ========== Phase 2: Physics Resolution (modifies ball directions) ==========

    /**
     * @brief Resolve ball-floor physics based on detected contacts
     *
     * Groups contacts by ball and determines bounce direction.
     * For multiple floor hits, analyzes floor alignment to handle:
     * - Horizontally aligned floors (same Y) → single horizontal surface
     * - Vertically aligned floors (same X) → single vertical surface
     * - L-shaped corners → bounce both directions
     */
    void resolveBallFloorPhysics(const Context& ctx, ContactList& contacts);

    /**
     * @brief Handle multi-floor collision by analyzing floor alignment
     * @param ball The ball that hit multiple floors
     * @param floorHits Vector of contacts with floors
     */
    void resolveMultiFloorCollision(Ball* ball, std::vector<Contact*>& floorHits);
};
