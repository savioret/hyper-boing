#include "collisionrules.h"
#include "scene.h"
#include "stageclear.h"
#include "../entities/ball.h"
#include "../entities/shot.h"
#include "../entities/player.h"
#include "floor.h"
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
                handleBallPlayer(c.getBall(), c.getPlayer());
                break;

            case ContactType::ShotFloor:
                handleShotFloor(c.getShot(), c.getFloor());
                break;

            case ContactType::BallFloor:
                // Physics already handled by CollisionSystem
                // No gameplay reaction needed
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

void CollisionRules::handleBallPlayer(Ball* ball, Player* player)
{
    // Skip if either already dead
    if (ball->isDead() || player->isDead()) return;

    // Fire PLAYER_HIT event for sound effects and other listeners
    GameEventData event(GameEventType::PLAYER_HIT);
    event.playerHit.player = player;
    event.playerHit.ball = ball;
    EVENT_MGR.trigger(event);

    // Kill the player (death animation handled by PlayerDeadAction)
    player->kill();
}

void CollisionRules::handleShotFloor(Shot* shot, Floor* floor)
{
    // Skip if shot already dead
    if (shot->isDead()) return;

    // Weapon-specific behavior (harpoon sticks, gun bullet explodes, etc.)
    shot->onFloorHit(floor);
}

int CollisionRules::calculateBallScore(int diameter)
{
    // Smaller balls = higher score
    return 1000 / diameter;
}
