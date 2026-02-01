#include "collisionsystem.h"
#include "../entities/ball.h"
#include "../entities/shot.h"
#include "../entities/player.h"
#include "floor.h"

ContactList CollisionSystem::detectAndResolve(Context& ctx)
{
    ContactList contacts;
    contacts.reserve(32);  // Pre-allocate for typical frame

    // Detect all collisions
    detectBallVsShot(ctx, contacts);
    detectBallVsFloor(ctx, contacts);

    if (ctx.checkPlayerCollisions)
    {
        detectBallVsPlayer(ctx, contacts);
    }

    detectShotVsFloor(ctx, contacts);

    return contacts;
}

void CollisionSystem::detectBallVsShot(Context& ctx, ContactList& contacts)
{
    for (const auto& b : ctx.balls)
    {
        if (b->isDead()) continue;

        for (const auto& sh : ctx.shots)
        {
            if (sh->isDead()) continue;
            if (sh->getPlayer()->isDead()) continue;

            if (b->collision(sh.get()))
            {
                // Record the contact - CollisionRules will handle scoring, events, killing
                contacts.push_back({
                    ContactType::BallShot,
                    b.get(),   // entityA = ball (raw pointer for observer)
                    sh.get()   // entityB = shot (raw pointer for observer)
                });

                break;  // Ball can only be hit once per frame
            }
        }
    }
}

void CollisionSystem::detectBallVsFloor(Context& ctx, ContactList& contacts)
{
    for (const auto& b : ctx.balls)
    {
        if (b->isDead()) continue;

        FloorColision flc[2];
        int cont = 0;
        int moved = 0;

        for (const auto& fl : ctx.floors)
        {
            SDL_Point col = b->collision(fl.get());

            if (col.x)
            {
                if (cont && flc[0].floor == fl.get())
                {
                    b->setDirX(-b->getDirX());
                    moved = 1;
                    break;
                }
                if (cont < 2)
                {
                    flc[cont].point.x = col.x;
                    flc[cont].floor = fl.get();
                    cont++;
                }
            }
            if (col.y)
            {
                if (cont && flc[0].floor == fl.get())
                {
                    b->setDirY(-b->getDirY());
                    moved = 2;
                    break;
                }
                if (cont < 2)
                {
                    flc[cont].point.y = col.y;
                    flc[cont].floor = fl.get();
                    cont++;
                }
            }
        }

        // Resolve physics immediately
        if (cont == 1)
        {
            if (flc[0].point.x)
                b->setDirX(-b->getDirX());
            else
                b->setDirY(-b->getDirY());

            // Record contact (optional - currently no gameplay reaction)
            contacts.push_back({
                ContactType::BallFloor,
                b.get(),
                flc[0].floor
            });
        }
        else if (cont > 1)
        {
            resolveBallFloorCollision(b.get(), flc, moved);

            // Record contact for first floor
            contacts.push_back({
                ContactType::BallFloor,
                b.get(),
                flc[0].floor
            });
        }
    }
}

void CollisionSystem::detectBallVsPlayer(Context& ctx, ContactList& contacts)
{
    for (const auto& b : ctx.balls)
    {
        if (b->isDead()) continue;

        for (int i = 0; i < 2; i++)
        {
            if (!ctx.players[i]) continue;
            if (ctx.players[i]->isImmune()) continue;
            if (ctx.players[i]->isDead()) continue;

            if (b->collision(ctx.players[i]))
            {
                // Record the contact - CollisionRules will handle events and killing
                contacts.push_back({
                    ContactType::BallPlayer,
                    b.get(),         // entityA = ball (raw pointer for observer)
                    ctx.players[i]   // entityB = player
                });
            }
        }
    }
}

void CollisionSystem::detectShotVsFloor(Context& ctx, ContactList& contacts)
{
    for (const auto& sh : ctx.shots)
    {
        if (sh->isDead()) continue;

        for (const auto& fl : ctx.floors)
        {
            if (sh->collision(fl.get()))
            {
                // Record the contact - CollisionRules will call onFloorHit
                contacts.push_back({
                    ContactType::ShotFloor,
                    sh.get(),  // entityA = shot (raw pointer for observer)
                    fl.get()   // entityB = floor (raw pointer for observer)
                });

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
