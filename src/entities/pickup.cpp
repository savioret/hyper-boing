#include "pickup.h"
#include "player.h"
#include "../game/stageclear.h"  // Must come before scene.h for unique_ptr<StageClear>
#include "../game/scene.h"
#include "../game/stage.h"
#include "../core/appdata.h"
#include "../core/graph.h"
#include "../core/logger.h"

Pickup::Pickup(Scene* scene, int x, int y, PickupType type)
    : scene(scene), pickupType(type), falling(true), groundY(Stage::MAX_Y)
{
    xPos = static_cast<float>(x);
    yPos = static_cast<float>(y);
    xInit = xPos;
    yInit = yPos;

    // Set sprite from shared resources
    StageResources& res = AppData::instance().getStageRes();
    sprite.setSprite(&res.pickupSprites[static_cast<int>(type)]);
}

void Pickup::update(float dt)
{
    if (isDead()) return;

    if (falling)
    {
        // Linear fall (not affected by time freeze)
        yPos += FALL_SPEED;

        // Stop at ground level
        if (yPos >= groundY)
        {
            yPos = groundY;
            falling = false;
        }
    }
}

void Pickup::draw(Graph* graph)
{
    if (isDead()) return;

    Sprite* spr = sprite.getSprite();
    if (spr)
    {
        // Draw at top-left (convert from bottom-middle pivot)
        int drawX = static_cast<int>(xPos) - SPRITE_SIZE / 2;
        int drawY = static_cast<int>(yPos) - SPRITE_SIZE;
        graph->draw(spr, drawX, drawY);
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
            // Allow 2 harpoon shots
            player->setExtraShots(1);
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
            // Placeholder - not implemented
            LOG_INFO("Player %d collected CLAW pickup (not implemented)", player->getId() + 1);
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
