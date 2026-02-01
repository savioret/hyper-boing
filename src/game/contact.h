#pragma once

#include <vector>

// Forward declarations
class Ball;
class Shot;
class Floor;
class Player;

/**
 * @enum ContactType
 * @brief Types of collisions that can occur in the game
 */
enum class ContactType
{
    BallFloor,    ///< Ball bounced off a floor/wall (physics already resolved)
    BallShot,     ///< Ball was hit by a shot (needs gameplay handling)
    BallPlayer,   ///< Ball hit a player (needs gameplay handling)
    ShotFloor     ///< Shot hit a floor/ceiling (needs gameplay handling)
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
    // BallFloor:  ball, floor
    // BallShot:   ball, shot
    // BallPlayer: ball, player
    // ShotFloor:  shot, floor
    void* entityA;
    void* entityB;

    // Convenience accessors with type safety
    Ball* getBall() const { return static_cast<Ball*>(entityA); }
    Shot* getShot() const { 
        // For BallShot: shot is in entityB
        // For ShotFloor: shot is in entityA
        return type == ContactType::ShotFloor 
            ? static_cast<Shot*>(entityA) 
            : static_cast<Shot*>(entityB); 
    }
    Floor* getFloor() const { return static_cast<Floor*>(entityB); }
    Player* getPlayer() const { return static_cast<Player*>(entityB); }
};

/**
 * @typedef ContactList
 * @brief A list of contacts detected in a single frame
 */
using ContactList = std::vector<Contact>;
