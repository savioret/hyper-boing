#include "collisionrules.h"
#include "scene.h"
#include "stageclear.h"
#include "../entities/ball.h"
#include "../entities/hexa.h"
#include "../entities/shot.h"
#include "../entities/player.h"
#include "../entities/pickup.h"
#include "platform.h"
#include "../core/eventmanager.h"

void CollisionRules::processContacts(const ContactList& contacts, Scene* scene)
{
    for (const Contact& c : contacts)
    {
        switch (c.type)
        {
            case ContactType::BallShot:
                handleBallShot(c.getBall(), c.getShot());
                break;

            case ContactType::BallPlayer:
                handleBallPlayer(c.getBall(), c.getPlayer(), scene);
                break;

            case ContactType::HexaShot:
                handleHexaShot(c.getHexa(), c.getShot());
                break;

            case ContactType::HexaPlayer:
                handleHexaPlayer(c.getHexa(), c.getPlayer(), scene);
                break;

            case ContactType::ShotFloor:
                handleShotFloor(c.getShot(), c.getPlatform());
                break;

            case ContactType::BallFloor:
            case ContactType::HexaFloor:
                // Physics already handled by CollisionSystem
                // No gameplay reaction needed
                break;

            case ContactType::PickupPlayer:
                handlePickupPlayer(c.getPickup(), c.getPlayer());
                break;
        }
    }
}

void CollisionRules::handleBallShot(Ball* ball, Shot* shot)
{
    // Skip if either already dead (from earlier contact this frame)
    if (ball->isDead() || shot->isDead()) return;

    // Weapon-specific behavior (harpoon retracts, gun explodes, etc.)
    shot->onBallHit(ball);

    // Award score to shooting player
    Player* shooter = shot->getPlayer();
    shooter->addScore(calculateBallScore(ball->getDiameter()));

    // Fire BALL_HIT event for sound effects and other listeners
    GameEventData event(GameEventType::BALL_HIT);
    event.ballHit.ball = ball;
    event.ballHit.shot = shot;
    event.ballHit.shooter = shooter;
    EVENT_MGR.trigger(event);

    // Mark ball for death (will split into children during cleanup)
    ball->kill();
}

void CollisionRules::handleBallPlayer(Ball* ball, Player* player, Scene* scene)
{
    // Skip if either already dead
    if (ball->isDead() || player->isDead()) return;

    // Skip if time is frozen (TIME_FREEZE pickup is active)
    if (scene->isTimeFrozen()) return;

    // Check if player has shield
    if (player->getShield())
    {
        // Consume shield instead of dying
        player->setShield(false);
        // Grant brief immunity so they don't die immediately
        player->setImmuneCounter(120);  // ~2 seconds
        return;
    }

    // Fire PLAYER_HIT event for sound effects and other listeners
    GameEventData event(GameEventType::PLAYER_HIT);
    event.playerHit.player = player;
    event.playerHit.ball = ball;
    EVENT_MGR.trigger(event);

    // Kill the player (death animation handled by PlayerDeadAction)
    player->kill();
}

void CollisionRules::handleShotFloor(Shot* shot, Platform* platform)
{
    // Skip if shot already dead
    if (shot->isDead()) return;

    // Weapon-specific behavior (harpoon sticks, gun bullet explodes, etc.)
    shot->onFloorHit(platform);

    // Advance glass damage state (no-op for regular floors)
    platform->onHit();
}

void CollisionRules::handlePickupPlayer(Pickup* pickup, Player* player)
{
    // Skip if either already dead
    if (pickup->isDead() || player->isDead()) return;

    // Apply the pickup effect to the player
    pickup->applyEffect(player);

    // Fire PICKUP_COLLECTED event
    GameEventData event(GameEventType::PICKUP_COLLECTED);
    event.pickupCollected.pickup = pickup;
    event.pickupCollected.player = player;
    event.pickupCollected.type = pickup->getType();
    EVENT_MGR.trigger(event);

    // Mark pickup for deletion
    pickup->kill();
}

int CollisionRules::calculateBallScore(int diameter)
{
    // Smaller balls = higher score
    return 1000 / diameter;
}

void CollisionRules::handleHexaShot(Hexa* hexa, Shot* shot)
{
    // Skip if either already dead (from earlier contact this frame)
    if (hexa->isDead() || shot->isDead()) return;

    // Weapon-specific behavior (harpoon retracts, gun explodes, etc.)
    // Note: onBallHit works for hexas too - it's about weapon behavior
    shot->onBallHit(nullptr);  // Pass nullptr since hexa isn't a Ball*

    // Award score to shooting player
    Player* shooter = shot->getPlayer();
    shooter->addScore(calculateHexaScore(hexa->getWidth()));

    // Fire HEXA_HIT event for sound effects and other listeners
    GameEventData event(GameEventType::HEXA_HIT);
    event.hexaHit.hexa = hexa;
    event.hexaHit.shot = shot;
    event.hexaHit.shooter = shooter;
    EVENT_MGR.trigger(event);

    // Mark hexa for death (will split into children during cleanup)
    hexa->kill();
}

void CollisionRules::handleHexaPlayer(Hexa* hexa, Player* player, Scene* scene)
{
    // Skip if either already dead
    if (hexa->isDead() || player->isDead()) return;

    // Skip if time is frozen (TIME_FREEZE pickup is active)
    if (scene->isTimeFrozen()) return;

    // Check if player has shield
    if (player->getShield())
    {
        // Consume shield instead of dying
        player->setShield(false);
        // Grant brief immunity so they don't die immediately
        player->setImmuneCounter(120);  // ~2 seconds
        return;
    }

    // Fire PLAYER_HIT event for sound effects and other listeners
    GameEventData event(GameEventType::PLAYER_HIT);
    event.playerHit.player = player;
    event.playerHit.ball = nullptr;  // It's a hexa, not a ball
    EVENT_MGR.trigger(event);

    // Kill the player (death animation handled by PlayerDeadAction)
    player->kill();
}

int CollisionRules::calculateHexaScore(int width)
{
    // Smaller hexas = higher score (similar to balls)
    return 1000 / width;
}
