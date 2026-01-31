#include "collisionsystem.h"
#include "../entities/ball.h"
#include "../entities/shot.h"
#include "../entities/player.h"
#include "floor.h"
#include "../core/appdata.h"
#include "../core/eventmanager.h"

void CollisionSystem::checkAll(Context& ctx)
{
    checkBallVsShot(ctx);
    checkBallVsFloor(ctx);

    if (ctx.checkPlayerCollisions)
    {
        checkBallVsPlayer(ctx);
    }

    checkShotVsFloor(ctx);
}

void CollisionSystem::checkBallVsShot(Context& ctx)
{
    for (Ball* b : ctx.balls)
    {
        if (b->isDead()) continue;

        for (Shot* sh : ctx.shots)
        {
            if (sh->isDead()) continue;
            if (sh->getPlayer()->isDead()) continue;

            if (b->collision(sh))
            {
                sh->onBallHit(b);  // Polymorphic call - handles weapon-specific behavior
                sh->getPlayer()->addScore(objectScore(b->getDiameter()));

                // Fire BALL_HIT event
                GameEventData event(GameEventType::BALL_HIT);
                event.ballHit.ball = b;
                event.ballHit.shot = sh;
                event.ballHit.shooter = sh->getPlayer();
                EVENT_MGR.trigger(event);

                b->kill();
                break;  // Ball can only be hit once per frame
            }
        }
    }
}

void CollisionSystem::checkBallVsFloor(Context& ctx)
{
    for (Ball* b : ctx.balls)
    {
        if (b->isDead()) continue;

        FloorColision flc[2];
        int cont = 0;
        int moved = 0;

        for (Floor* fl : ctx.floors)
        {
            SDL_Point col = b->collision(fl);

            if (col.x)
            {
                if (cont && flc[0].floor == fl)
                {
                    b->setDirX(-b->getDirX());
                    moved = 1;
                    break;
                }
                if (cont < 2)
                {
                    flc[cont].point.x = col.x;
                    flc[cont].floor = fl;
                    cont++;
                }
            }
            if (col.y)
            {
                if (cont && flc[0].floor == fl)
                {
                    b->setDirY(-b->getDirY());
                    moved = 2;
                    break;
                }
                if (cont < 2)
                {
                    flc[cont].point.y = col.y;
                    flc[cont].floor = fl;
                    cont++;
                }
            }
        }

        // Resolve collision based on how many floors were hit
        if (cont == 1)
        {
            if (flc[0].point.x)
                b->setDirX(-b->getDirX());
            else
                b->setDirY(-b->getDirY());
        }
        else if (cont > 1)
        {
            resolveBallFloorCollision(b, flc, moved);
        }
    }
}

void CollisionSystem::checkBallVsPlayer(Context& ctx)
{
    for (Ball* b : ctx.balls)
    {
        if (b->isDead()) continue;

        for (int i = 0; i < 2; i++)
        {
            if (!ctx.players[i]) continue;
            if (ctx.players[i]->isImmune()) continue;
            if (ctx.players[i]->isDead()) continue;

            if (b->collision(ctx.players[i]))
            {
                // Fire PLAYER_HIT event
                GameEventData event(GameEventType::PLAYER_HIT);
                event.playerHit.player = ctx.players[i];
                event.playerHit.ball = b;
                EVENT_MGR.trigger(event);

                ctx.players[i]->kill();
                // Frame is now set by PlayerDeadAction via event subscription
            }
        }
    }
}

void CollisionSystem::checkShotVsFloor(Context& ctx)
{
    for (Shot* sh : ctx.shots)
    {
        if (sh->isDead()) continue;

        for (Floor* fl : ctx.floors)
        {
            if (sh->collision(fl))
            {
                sh->onFloorHit(fl);  // Polymorphic call - handles weapon-specific behavior
                break;  // Shot can only hit one floor per frame
            }
        }
    }
}

void CollisionSystem::resolveBallFloorCollision(Ball* b, FloorColision* fc, int moved)
{
    if (fc[0].floor->getId() == fc[1].floor->getId() || fc[0].point.y == fc[1].point.y)
    {
        if (fc[0].point.x)
            if (moved != 1) b->setDirX(-b->getDirX());

        if (fc[0].point.y)
            if (moved != 2) b->setDirY(-b->getDirY());
        return;
    }

    if (fc[0].floor->getId() == fc[1].floor->getId())
    {
        if (fc[0].floor->getId() == 0)
            if (moved != 2) b->setDirY(-b->getDirY());
        if (fc[0].floor->getId() == 1)
            if (moved != 1) b->setDirX(-b->getDirX());
    }
    else
    {
        if (fc[0].floor->getY() == fc[1].floor->getY())
            if (moved != 2) b->setDirY(-b->getDirY());
            else
                if (moved != 1) b->setDirX(-b->getDirX());
    }
}

int CollisionSystem::objectScore(int diameter)
{
    return 1000 / diameter;
}
