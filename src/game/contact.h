#pragma once

#include <vector>
#include "../core/collisionbox.h"

// Forward declarations - Contact stores only pointers to these types
class IGameObject;
class Ball;
class Hexa;
class Shot;
class Platform;
class Player;
class Pickup;

/**
 * @enum ContactType
 * @brief Types of collisions that can occur in the game
 */
enum class ContactType
{
    BallFloor,    ///< Ball bounced off a floor/wall (physics already resolved)
    BallShot,     ///< Ball was hit by a shot (needs gameplay handling)
    BallPlayer,   ///< Ball hit a player (needs gameplay handling)
    HexaFloor,    ///< Hexa bounced off a floor/wall (needs physics resolution)
    HexaShot,     ///< Hexa was hit by a shot (needs gameplay handling)
    HexaPlayer,   ///< Hexa hit a player (needs gameplay handling)
    ShotFloor,    ///< Shot hit a floor/ceiling (needs gameplay handling)
    PickupPlayer  ///< Pickup collected by player (needs gameplay handling)
};

/**
 * @struct Contact
 * @brief Records a collision between two entities
 *
 * This is the "data glue" between collision detection (physics)
 * and game rules (gameplay). The CollisionSystem produces contacts,
 * and CollisionRules consumes them.
 */
struct Contact
{
    ContactType type;

    // Entity pointers - interpretation depends on type:
    // BallFloor:  ball, platform
    // BallShot:   ball, shot
    // BallPlayer: ball, player
    // ShotFloor:  shot, platform
    IGameObject* entityA;
    IGameObject* entityB;

    // Collision boxes captured at detection time
    CollisionBox boxA;
    CollisionBox boxB;

    // Collision side - which side of B was hit by A (used for physics resolution)
    // For BallFloor: which side of the floor the ball hit
    CollisionSide side;

    // Convenience accessors with type safety
    // Note: Using reinterpret_cast because forward declarations prevent static_cast
    Ball* getBall() const { return reinterpret_cast<Ball*>(entityA); }
    Hexa* getHexa() const { return reinterpret_cast<Hexa*>(entityA); }
    Shot* getShot() const {
        // For BallShot/HexaShot: shot is in entityB
        // For ShotFloor: shot is in entityA
        return type == ContactType::ShotFloor
            ? reinterpret_cast<Shot*>(entityA)
            : reinterpret_cast<Shot*>(entityB);
    }
    Platform* getPlatform() const { return reinterpret_cast<Platform*>(entityB); }
    Player* getPlayer() const {
        // For PickupPlayer: player is in entityB
        // For BallPlayer/HexaPlayer: player is in entityB
        return reinterpret_cast<Player*>(entityB);
    }
    Pickup* getPickup() const {
        // For PickupPlayer: pickup is in entityA
        return reinterpret_cast<Pickup*>(entityA);
    }

    /**
     * @brief Get the contact point (center of overlap between collision boxes)
     * @param outX Output x coordinate of contact point
     * @param outY Output y coordinate of contact point
     * @return true if overlap exists, false otherwise
     *
     * Use this to determine where to spawn visual effects (sparks, particles, etc.)
     * when a collision occurs. Returns the center of the overlapping region.
     */
    bool getContactPoint(int& outX, int& outY) const {
        return getOverlapCenter(boxA, boxB, outX, outY);
    }

    /**
     * @brief Get the overlap rectangle for this contact
     * @return CollisionBox representing the overlap region
     *
     * Useful if you need more than just the center point, such as
     * for spawning multiple particles within the collision area.
     */
    CollisionBox getOverlapRect() const {
        return ::getOverlapRect(boxA, boxB);
    }
};

/**
 * @typedef ContactList
 * @brief A list of contacts detected in a single frame
 */
using ContactList = std::vector<Contact>;
