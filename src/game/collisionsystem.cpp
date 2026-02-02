#include "collisionsystem.h"
#include "../entities/ball.h"
#include "../entities/shot.h"
#include "../entities/player.h"
#include "floor.h"
#include <unordered_map>
#include <vector>

ContactList CollisionSystem::detectAndResolve(Context& ctx)
{
    ContactList contacts;
    contacts.reserve(32);  // Pre-allocate for typical frame

    // Phase 1: Detection (const - only builds contacts)
    detectBallVsFloor(ctx, contacts);
    detectBallVsShot(ctx, contacts);

    if (ctx.checkPlayerCollisions)
    {
        detectBallVsPlayer(ctx, contacts);
    }

    detectShotVsFloor(ctx, contacts);

    // Phase 2: Physics resolution (modifies ball directions)
    resolveBallFloorPhysics(ctx, contacts);

    return contacts;
}

void CollisionSystem::detectBallVsShot(const Context& ctx, ContactList& contacts) const
{
    for (const auto& b : ctx.balls)
    {
        if (b->isDead()) continue;

        for (const auto& sh : ctx.shots)
        {
            if (sh->isDead()) continue;
            if (sh->getPlayer()->isDead()) continue;

            CollisionBox ballBox = b->getCollisionBox();
            CollisionBox shotBox = sh->getCollisionBox();

            if (intersects(ballBox, shotBox))
            {
                contacts.push_back({
                    ContactType::BallShot,
                    b.get(),
                    sh.get(),
                    ballBox,
                    shotBox,
                    getCollisionSide(ballBox, shotBox)
                });

                break;  // Ball can only be hit once per frame
            }
        }
    }
}

void CollisionSystem::detectBallVsFloor(const Context& ctx, ContactList& contacts) const
{
    for (const auto& b : ctx.balls)
    {
        if (b->isDead()) continue;

        CollisionBox ballBox = b->getCollisionBox();
        int dirX = b->getDirX();
        int dirY = b->getDirY();

        for (const auto& fl : ctx.floors)
        {
            CollisionBox floorBox = fl->getCollisionBox();

            if (!intersects(ballBox, floorBox))
                continue;

            CollisionSide side = getCollisionSide(ballBox, floorBox);

            // Check if ball is fully contained inside floor (emergency escape)
            bool ballInsideFloor = containsBox(floorBox, ballBox);

            // Only register collision if ball is moving TOWARD that side
            // This prevents "sticking" when ball grazes a surface while moving parallel
            // Exception: if ball is fully inside floor, always register to escape
            bool validX = side.x && ((side.x == SIDE_LEFT && dirX == 1) ||
                                     (side.x == SIDE_RIGHT && dirX == -1) ||
                                     ballInsideFloor);
            bool validY = side.y && ((side.y == SIDE_TOP && dirY == 1) ||
                                     (side.y == SIDE_BOTTOM && dirY == -1) ||
                                     ballInsideFloor);

            if (validX || validY)
            {
                // Record only the valid collision sides
                CollisionSide recordedSide;
                if (validX) recordedSide.x = side.x;
                if (validY) recordedSide.y = side.y;

                contacts.push_back({
                    ContactType::BallFloor,
                    b.get(),
                    fl.get(),
                    ballBox,
                    floorBox,
                    recordedSide
                });
            }
        }
    }
}

void CollisionSystem::detectBallVsPlayer(const Context& ctx, ContactList& contacts) const
{
    for (const auto& b : ctx.balls)
    {
        if (b->isDead()) continue;

        for (int i = 0; i < 2; i++)
        {
            if (!ctx.players[i]) continue;
            if (ctx.players[i]->isImmune()) continue;
            if (ctx.players[i]->isDead()) continue;

            CollisionBox ballBox = b->getCollisionBox();
            CollisionBox playerBox = ctx.players[i]->getCollisionBox();

            if (intersects(ballBox, playerBox))
            {
                contacts.push_back({
                    ContactType::BallPlayer,
                    b.get(),
                    ctx.players[i],
                    ballBox,
                    playerBox,
                    getCollisionSide(ballBox, playerBox)
                });
            }
        }
    }
}

void CollisionSystem::detectShotVsFloor(const Context& ctx, ContactList& contacts) const
{
    for (const auto& sh : ctx.shots)
    {
        if (sh->isDead()) continue;

        for (const auto& fl : ctx.floors)
        {
            CollisionBox shotBox = sh->getCollisionBox();
            CollisionBox floorBox = fl->getCollisionBox();

            if (intersects(shotBox, floorBox))
            {
                contacts.push_back({
                    ContactType::ShotFloor,
                    sh.get(),
                    fl.get(),
                    shotBox,
                    floorBox,
                    getCollisionSide(shotBox, floorBox)
                });

                break;  // Shot can only hit one floor per frame
            }
        }
    }
}

void CollisionSystem::resolveBallFloorPhysics(const Context& ctx, ContactList& contacts)
{
    // Group BallFloor contacts by ball pointer
    std::unordered_map<Ball*, std::vector<Contact*>> ballContacts;

    for (auto& c : contacts)
    {
        if (c.type == ContactType::BallFloor)
        {
            ballContacts[c.getBall()].push_back(&c);
        }
    }

    // Resolve physics for each ball
    for (auto& pair : ballContacts)
    {
        Ball* ball = pair.first;
        std::vector<Contact*>& floorHits = pair.second;

        if (floorHits.size() == 1)
        {
            // Single floor hit - simple bounce based on collision side
            const CollisionSide& side = floorHits[0]->side;
            if (side.hasHorizontal()) ball->setDirX(-ball->getDirX());
            if (side.hasVertical()) ball->setDirY(-ball->getDirY());
        }
        else
        {
            // Multiple floors hit - analyze alignment to determine bounce
            resolveMultiFloorCollision(ball, floorHits);
        }
    }
}

void CollisionSystem::resolveMultiFloorCollision(Ball* ball, std::vector<Contact*>& floorHits)
{
    // Merge collision sides from all floor contacts
    // Key insight: aligned floors form a single logical surface
    //
    // SCENARIO: Horizontally aligned floors (same Y position)
    //   ┌────────┐ ┌────────┐
    //   │ Floor1 │ │ Floor2 │   <- Same Y, form horizontal surface
    //   └────────┘ └────────┘
    //   Ball hitting from above should only bounce Y, not X
    //
    // SCENARIO: Vertically aligned floors (same X position)
    //   ┌────────┐
    //   │ Floor1 │
    //   └────────┘
    //   ┌────────┐
    //   │ Floor2 │   <- Same X, form vertical surface
    //   └────────┘
    //   Ball hitting from side should only bounce X, not Y
    //
    // SCENARIO: Corner (L-shaped arrangement)
    //   Ball should bounce both X and Y

    bool bounceX = false;
    bool bounceY = false;

    // Check if all floors are aligned (share same Y or same X)
    bool allSameY = true;
    bool allSameX = true;
    int firstY = floorHits[0]->boxB.y;
    int firstX = floorHits[0]->boxB.x;

    for (size_t i = 1; i < floorHits.size(); i++)
    {
        if (floorHits[i]->boxB.y != firstY) allSameY = false;
        if (floorHits[i]->boxB.x != firstX) allSameX = false;
    }

    // Collect all collision sides
    bool hasHorizontalHit = false;
    bool hasVerticalHit = false;

    for (const auto* contact : floorHits)
    {
        if (contact->side.hasHorizontal()) hasHorizontalHit = true;
        if (contact->side.hasVertical()) hasVerticalHit = true;
    }

    if (allSameY && hasVerticalHit)
    {
        // Floors are horizontally aligned - they form a horizontal surface
        // Only bounce vertically, ignore any spurious horizontal collisions
        bounceY = true;
    }
    else if (allSameX && hasHorizontalHit)
    {
        // Floors are vertically aligned - they form a vertical surface
        // Only bounce horizontally, ignore any spurious vertical collisions
        bounceX = true;
    }
    else
    {
        // Floors form a corner (L-shape or other arrangement)
        // Bounce based on actual collision sides detected
        bounceX = hasHorizontalHit;
        bounceY = hasVerticalHit;
    }

    // Apply bounces
    if (bounceX) ball->setDirX(-ball->getDirX());
    if (bounceY) ball->setDirY(-ball->getDirY());
}
