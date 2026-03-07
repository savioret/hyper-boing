#pragma once

#include "contact.h"

// Forward declarations
class Scene;
class Ball;
class Shot;
class Platform;
class Player;
class Pickup;

/**
 * @class CollisionRules
 * @brief Processes collision contacts and applies game logic
 *
 * This class is the "judge" that interprets what collisions mean
 * for the game rules. It handles:
 * - Scoring when balls are hit
 * - Player death when hit by balls
 * - Weapon-specific behaviors
 * - Firing gameplay events
 *
 * The separation from CollisionSystem allows physics and gameplay
 * to evolve independently.
 */
class CollisionRules
{
public:
    /**
     * @brief Process all contacts from this frame
     * @param contacts List of collisions detected by CollisionSystem
     * @param scene Scene context for accessing entities and firing events
     */
    void processContacts(const ContactList& contacts, Scene* scene);

private:
    /**
     * @brief Handle ball-shot collision
     *
     * - Calls shot->onBallHit() for weapon-specific behavior
     * - Awards score to shooting player
     * - Fires BALL_HIT event
     * - Marks ball for death (will split in cleanup)
     */
    void handleBallShot(Ball* ball, Shot* shot);

    /**
     * @brief Handle ball-player collision
     *
     * - Skips if time is frozen (TIME_FREEZE pickup is active)
     * - Fires PLAYER_HIT event
     * - Marks player for death
     */
    void handleBallPlayer(Ball* ball, Player* player, Scene* scene);

    /**
     * @brief Handle shot-platform collision
     *
     * - Calls shot->onFloorHit() for weapon-specific behavior
     * - Calls platform->onHit() to degrade Glass (no-op for Floor)
     */
    void handleShotFloor(Shot* shot, Platform* platform);

    /**
     * @brief Handle pickup-player collision
     *
     * - Calls pickup->applyEffect() to apply power-up
     * - Fires PICKUP_COLLECTED event
     * - Marks pickup for death
     */
    void handlePickupPlayer(Pickup* pickup, Player* player);

    /**
     * @brief Calculate score for destroying a ball
     * @param diameter Ball diameter
     * @return Score value (larger balls = lower score)
     */
    static int calculateBallScore(int diameter);
};
