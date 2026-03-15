#include "pickup.h"
#include "player.h"
#include "../game/stageclear.h"  // Must come before scene.h for unique_ptr<StageClear>
#include "../game/scene.h"
#include "../game/stage.h"
#include "../core/appdata.h"
#include "../core/graph.h"
#include "../core/logger.h"
#include "../core/coordhelper.h"

Pickup::Pickup(Scene* scene, int x, int y, PickupType type)
    : scene(scene), pickupType(type), falling(true), groundY(Stage::MAX_Y), groundTimer(0.0f)
{
    xPos = static_cast<float>(x);
    yPos = static_cast<float>(y);
    xInit = xPos;
    yInit = yPos;

    // Set sprite from shared resources
    StageResources& res = AppData::instance().getStageRes();
    if (type == PickupType::SHIELD)
    {
        if (res.pickupShieldAnim)
            shieldAnim = res.pickupShieldAnim->clone();
    }
    else
    {
        // SHIELD (enum 5) is handled above; CLAW (enum 6) maps to array index 5
        int idx = (type == PickupType::CLAW) ? 5 : static_cast<int>(type);
        sprite.setSprite(&res.pickupSprites[idx]);
    }
}

float Pickup::findGroundBelow() const
{
    if (!scene)
        return (float)Stage::MAX_Y+1;

    // Use a narrow strip (10% of sprite width) centered at xPos.
    // A full-width check would cause pickups to land on platforms they barely touch at the edge.
    int halfStrip = std::max(1, (int)(SPRITE_SIZE * 0.05f));  // 5% each side = 10% total
    int stripLeft  = (int)xPos - halfStrip;
    int stripRight = (int)xPos + halfStrip;

    float bestY = (float)Stage::MAX_Y+1;

    for (const auto& floor : scene->lsFloor)
    {
        if (floor->isDead() || floor->isInvisible())
            continue;

        CollisionBox box = floor->getCollisionBox();

        // The pickup's center strip must overlap the platform horizontally
        if (stripRight < box.x || stripLeft > box.x + box.w)
            continue;

        // Platform top must be at or below the pickup's current bottom (yPos)
        if ((float)box.y < yPos)
            continue;

        if ((float)box.y < bestY)
            bestY = (float)box.y;
    }

    return bestY;
}

void Pickup::update(float dt)
{
    if (isDead()) return;

    if (shieldAnim)
        shieldAnim->update(dt * 1000.0f);

    if (!falling)
    {
        // Check if the platform we're resting on is still alive
        if (landedPlatform && (landedPlatform->isDead() || landedPlatform->isInvisible()))
        {
            // Platform is gone � resume falling
            falling = true;
            landedPlatform = nullptr;
            groundY = (float)Stage::MAX_Y;
        }
    }

    if (falling)
    {
        yPos += FALL_SPEED;

        // Find nearest platform below on every step so we don't overshoot
        float platformY = findGroundBelow();
        groundY = platformY;

        if (yPos >= groundY)
        {
            yPos = groundY;
            falling = false;

            // Record which platform we landed on (nullptr means the hard floor)
            landedPlatform = nullptr;
            if (groundY < (float)Stage::MAX_Y && scene)
            {
                int halfStrip = std::max(1, (int)(SPRITE_SIZE * 0.05f));
                int stripLeft  = (int)xPos - halfStrip;
                int stripRight = (int)xPos + halfStrip;
                for (const auto& floor : scene->lsFloor)
                {
                    if (floor->isDead() || floor->isInvisible())
                        continue;
                    CollisionBox box = floor->getCollisionBox();
                    if (stripRight >= box.x && stripLeft <= box.x + box.w && (float)box.y == groundY)
                    {
                        landedPlatform = floor.get();
                        break;
                    }
                }
            }
        }
    }
    else
    {
        // Pickup is on ground - count down lifetime
        groundTimer += dt;
        if (groundTimer >= LIFETIME)
        {
            kill();
        }
    }
}

void Pickup::draw(Graph* graph)
{
    if (isDead()) return;

    // Blink during the last BLINK_DURATION seconds (toggle every 0.1 seconds)
    if (!falling && groundTimer >= (LIFETIME - BLINK_DURATION))
    {
        int blinkPhase = static_cast<int>(groundTimer * 10) % 2;
        if (blinkPhase == 0) return;  // Skip drawing every other 0.1 second
    }

    Sprite* spr = shieldAnim ? shieldAnim->getCurrentSprite() : sprite.getSprite();
    if (spr)
    {
        // Use the animation bounding box for stable positioning across frames;
        // fall back to SPRITE_SIZE for static pickups.
        int w = shieldAnim ? shieldAnim->getWidth()  : sprite.getSprite()->getWidth();
        int h = shieldAnim ? shieldAnim->getHeight() : sprite.getSprite()->getHeight();
        graph->draw(spr, toRenderX(xPos, w), toRenderY(yPos, h));
    }
}

void Pickup::applyEffect(Player* player)
{
    if (!player) return;

    switch (pickupType)
    {
        case PickupType::GUN:
            // Switch weapon to gun
            player->setWeapon(WeaponType::GUN);
            LOG_INFO("Player %d collected GUN pickup", player->getId() + 1);
            break;

        case PickupType::DOUBLE_SHOOT:
            // Switch to harpoon (in case player has gun) and allow 2 simultaneous shots
            player->setWeapon(WeaponType::HARPOON);
            player->setMaxShoots(2);
            LOG_INFO("Player %d collected DOUBLE_SHOOT pickup", player->getId() + 1);
            break;

        case PickupType::EXTRA_TIME:
            // Add 20 seconds to timer
            if (scene)
            {
                int currentTime = scene->getTimeRemaining();
                scene->setTimeRemaining(currentTime + static_cast<int>(EXTRA_TIME_BONUS));
                LOG_INFO("Player %d collected EXTRA_TIME pickup (+20 seconds)", player->getId() + 1);
            }
            break;

        case PickupType::TIME_FREEZE:
            // Freeze balls for 10 seconds (handled by Scene)
            if (scene)
            {
                scene->setTimeFrozen(true, FREEZE_DURATION);
                LOG_INFO("Player %d collected TIME_FREEZE pickup (%.0f seconds)", player->getId() + 1, FREEZE_DURATION);
            }
            break;

        case PickupType::EXTRA_LIFE:
            // Add 1 life
            player->setLives(player->getLives() + 1);
            LOG_INFO("Player %d collected EXTRA_LIFE pickup", player->getId() + 1);
            break;

        case PickupType::SHIELD:
            // Grant shield (survive 1 hit)
            player->setShield(true);
            LOG_INFO("Player %d collected SHIELD pickup", player->getId() + 1);
            break;

        case PickupType::CLAW:
            player->setWeapon(WeaponType::CLAW);
            LOG_INFO("Player %d collected CLAW pickup", player->getId() + 1);
            break;
    }
}

CollisionBox Pickup::getCollisionBox() const
{
    // Collision box is 16x16 centered at xPos, with bottom at yPos
    CollisionBox box;
    box.x = static_cast<int>(xPos) - SPRITE_SIZE / 2;
    box.y = static_cast<int>(yPos) - SPRITE_SIZE;
    box.w = SPRITE_SIZE;
    box.h = SPRITE_SIZE;
    return box;
}

const char* Pickup::getTypeName(PickupType type)
{
    switch (type)
    {
        case PickupType::GUN:          return "gun";
        case PickupType::DOUBLE_SHOOT: return "doubleshoot";
        case PickupType::EXTRA_TIME:   return "extratime";
        case PickupType::TIME_FREEZE:  return "timefreeze";
        case PickupType::EXTRA_LIFE:   return "1up";
        case PickupType::SHIELD:       return "shield";
        case PickupType::CLAW:         return "claw";
        default:                       return "unknown";
    }
}
