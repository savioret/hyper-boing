#include "collisionsystem.h"
#include "../entities/ball.h"
#include "../entities/hexa.h"
#include "../entities/shot.h"
#include "../entities/player.h"
#include "../entities/pickup.h"
#include "platform.h"
#include <unordered_map>
#include <vector>

ContactList CollisionSystem::detectAndResolve(Context& ctx)
{
    ContactList contacts;
    contacts.reserve(32);  // Pre-allocate for typical frame

    // Phase 1: Detection (const - only builds contacts)
    detectBallVsFloor(ctx, contacts);
    detectBallVsShot(ctx, contacts);
    detectHexaVsFloor(ctx, contacts);
    detectHexaVsShot(ctx, contacts);

    if (ctx.checkPlayerCollisions)
    {
        detectBallVsPlayer(ctx, contacts);
        detectHexaVsPlayer(ctx, contacts);
        detectPickupVsPlayer(ctx, contacts);
    }

    detectShotVsFloor(ctx, contacts);

    // Phase 2: Physics resolution (modifies ball/hexa directions)
    resolveBallFloorPhysics(ctx, contacts);
    resolveHexaFloorPhysics(ctx, contacts);

    return contacts;
}

void CollisionSystem::detectBallVsShot(const Context& ctx, ContactList& contacts) const
{
    for (const auto& b : ctx.balls)
    {
        // Skip dead OR flashing balls (flash = already hit, waiting for death)
        if (b->isDead() || b->isInFlashState()) continue;

        // Get ball circle collision data
        int ballCx, ballCy;
        b->getCollisionCenter(ballCx, ballCy);
        int ballRadius = b->getCollisionRadius();

        for (const auto& sh : ctx.shots)
        {
            if (sh->isDead()) continue;
            if (sh->getPlayer()->isDead()) continue;

            CollisionBox shotBox = sh->getCollisionBox();

            // Use circle-box collision for ball vs shot
            if (circleIntersectsBox(ballCx, ballCy, ballRadius, shotBox))
            {
                // Store the AABB in contact for compatibility with existing code
                CollisionBox ballBox = b->getCollisionBox();
                contacts.push_back({
                    ContactType::BallShot,
                    b.get(),
                    sh.get(),
                    ballBox,
                    shotBox,
                    getCircleBoxCollisionSide(ballCx, ballCy, ballRadius, shotBox)
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
        // Skip dead OR flashing balls
        if (b->isDead() || b->isInFlashState()) continue;

        // Get ball circle collision data
        int ballCx, ballCy;
        b->getCollisionCenter(ballCx, ballCy);
        int ballRadius = b->getCollisionRadius();
        CollisionBox ballBox = b->getCollisionBox();  // For Contact struct compatibility

        float dirX = b->getDirX();
        int dirY = b->getDirY();

        for (const auto& fl : ctx.floors)
        {
            CollisionBox floorBox = fl->getCollisionBox();

            // Use circle-box collision for ball vs floor
            if (!circleIntersectsBox(ballCx, ballCy, ballRadius, floorBox))
                continue;

            CollisionSide side = getCircleBoxCollisionSide(ballCx, ballCy, ballRadius, floorBox);

            // Check if ball center is inside floor (emergency escape)
            bool ballInsideFloor = contains(floorBox, (float)ballCx, (float)ballCy);

            // Only register collision if ball is moving TOWARD that side
            // This prevents "sticking" when ball grazes a surface while moving parallel
            // Exception: if ball is fully inside floor, always register to escape
            bool validX = side.x && ((side.x == SIDE_LEFT && dirX > 0) ||
                                     (side.x == SIDE_RIGHT && dirX < 0) ||
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
        // Skip dead OR flashing balls (flash = already hit, waiting for death)
        if (b->isDead() || b->isInFlashState()) continue;

        // Get ball circle collision data
        int ballCx, ballCy;
        b->getCollisionCenter(ballCx, ballCy);
        int ballRadius = b->getCollisionRadius();

        for (int i = 0; i < 2; i++)
        {
            if (!ctx.players[i]) continue;
            if (ctx.players[i]->isImmune()) continue;
            if (ctx.players[i]->isDead()) continue;

            CollisionBox playerBox = ctx.players[i]->getCollisionBox();

            // Use circle-box collision for ball vs player
            if (circleIntersectsBox(ballCx, ballCy, ballRadius, playerBox))
            {
                // Store the AABB in contact for compatibility with existing code
                CollisionBox ballBox = b->getCollisionBox();
                contacts.push_back({
                    ContactType::BallPlayer,
                    b.get(),
                    ctx.players[i],
                    ballBox,
                    playerBox,
                    getCircleBoxCollisionSide(ballCx, ballCy, ballRadius, playerBox)
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
            CollisionBox floorBox = fl->getCollisionBox();

            // Destructible platforms (Glass) use full shot collision box (like balls)
            // Regular floors use reduced collision box (tip to gun position)
            CollisionBox shotBox = fl->isDestructible()
                ? sh->getCollisionBox()
                : sh->getFloorCollisionBox();

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

void CollisionSystem::detectPickupVsPlayer(const Context& ctx, ContactList& contacts) const
{
    for (const auto& pickup : ctx.pickups)
    {
        if (pickup->isDead()) continue;

        for (int i = 0; i < 2; i++)
        {
            if (!ctx.players[i]) continue;
            if (ctx.players[i]->isDead()) continue;

            CollisionBox pickupBox = pickup->getCollisionBox();
            CollisionBox playerBox = ctx.players[i]->getCollisionBox();

            if (intersects(pickupBox, playerBox))
            {
                contacts.push_back({
                    ContactType::PickupPlayer,
                    pickup.get(),
                    ctx.players[i],
                    pickupBox,
                    playerBox,
                    {}  // No collision side needed for pickup
                });

                break;  // Pickup can only be collected by one player
            }
        }
    }
}

void CollisionSystem::detectHexaVsShot(const Context& ctx, ContactList& contacts) const
{
    for (const auto& h : ctx.hexas)
    {
        // Skip dead OR flashing hexas (flash = already hit, waiting for death)
        if (h->isDead() || h->isInFlashState()) continue;

        int hexaCx, hexaCy;
        h->getCollisionCenter(hexaCx, hexaCy);
        int hexaRadius = h->getCollisionRadius();

        for (const auto& sh : ctx.shots)
        {
            if (sh->isDead()) continue;
            if (sh->getPlayer()->isDead()) continue;

            CollisionBox shotBox = sh->getCollisionBox();

            if (circleIntersectsBox(hexaCx, hexaCy, hexaRadius, shotBox))
            {
                contacts.push_back({
                    ContactType::HexaShot,
                    h.get(),
                    sh.get(),
                    h->getCollisionBox(),
                    shotBox,
                    getCircleBoxCollisionSide(hexaCx, hexaCy, hexaRadius, shotBox)
                });

                break;  // Hexa can only be hit once per frame
            }
        }
    }
}

void CollisionSystem::detectHexaVsFloor(const Context& ctx, ContactList& contacts) const
{
    for (const auto& h : ctx.hexas)
    {
        // Skip dead OR flashing hexas
        if (h->isDead() || h->isInFlashState()) continue;

        int hexaCx, hexaCy;
        h->getCollisionCenter(hexaCx, hexaCy);
        int hexaRadius = h->getCollisionRadius();
        float velX = h->getVelX();
        float velY = h->getVelY();

        for (const auto& fl : ctx.floors)
        {
            CollisionBox floorBox = fl->getCollisionBox();

            if (!circleIntersectsBox(hexaCx, hexaCy, hexaRadius, floorBox))
                continue;

            CollisionSide side = getCircleBoxCollisionSide(hexaCx, hexaCy, hexaRadius, floorBox);

            // Check if hexa center is inside floor (emergency escape)
            bool hexaInsideFloor = (hexaCx >= floorBox.x && hexaCx <= floorBox.x + floorBox.w &&
                                    hexaCy >= floorBox.y && hexaCy <= floorBox.y + floorBox.h);

            // Only register collision if hexa is moving TOWARD that side
            bool validX = side.x && ((side.x == SIDE_LEFT && velX > 0) ||
                                     (side.x == SIDE_RIGHT && velX < 0) ||
                                     hexaInsideFloor);
            bool validY = side.y && ((side.y == SIDE_TOP && velY > 0) ||
                                     (side.y == SIDE_BOTTOM && velY < 0) ||
                                     hexaInsideFloor);

            if (validX || validY)
            {
                // Record only the valid collision sides
                CollisionSide recordedSide;
                if (validX) recordedSide.x = side.x;
                if (validY) recordedSide.y = side.y;

                contacts.push_back({
                    ContactType::HexaFloor,
                    h.get(),
                    fl.get(),
                    h->getCollisionBox(),
                    floorBox,
                    recordedSide
                });
            }
        }
    }
}

void CollisionSystem::detectHexaVsPlayer(const Context& ctx, ContactList& contacts) const
{
    for (const auto& h : ctx.hexas)
    {
        // Skip dead OR flashing hexas (flash = already hit, waiting for death)
        if (h->isDead() || h->isInFlashState()) continue;

        int hexaCx, hexaCy;
        h->getCollisionCenter(hexaCx, hexaCy);
        int hexaRadius = h->getCollisionRadius();

        for (int i = 0; i < 2; i++)
        {
            if (!ctx.players[i]) continue;
            if (ctx.players[i]->isImmune()) continue;
            if (ctx.players[i]->isDead()) continue;

            CollisionBox playerBox = ctx.players[i]->getCollisionBox();

            if (circleIntersectsBox(hexaCx, hexaCy, hexaRadius, playerBox))
            {
                contacts.push_back({
                    ContactType::HexaPlayer,
                    h.get(),
                    ctx.players[i],
                    h->getCollisionBox(),
                    playerBox,
                    getCircleBoxCollisionSide(hexaCx, hexaCy, hexaRadius, playerBox)
                });
            }
        }
    }
}

void CollisionSystem::resolveHexaFloorPhysics(const Context& ctx, ContactList& contacts)
{
    // Group HexaFloor contacts by hexa pointer
    std::unordered_map<Hexa*, std::vector<Contact*>> hexaContacts;

    for (auto& c : contacts)
    {
        if (c.type == ContactType::HexaFloor)
        {
            hexaContacts[c.getHexa()].push_back(&c);
        }
    }

    // Resolve physics for each hexa
    for (auto& pair : hexaContacts)
    {
        Hexa* hexa = pair.first;
        std::vector<Contact*>& floorHits = pair.second;

        // Merge collision sides from all floor contacts
        CollisionSide mergedSide;
        for (const auto* contact : floorHits)
        {
            if (contact->side.hasHorizontal()) mergedSide.x = contact->side.x;
            if (contact->side.hasVertical()) mergedSide.y = contact->side.y;
        }

        // Apply bounce via hexa's handlePlatformBounce
        hexa->handlePlatformBounce(mergedSide);
    }
}
