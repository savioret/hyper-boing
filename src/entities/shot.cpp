#include "main.h"
#include "shot.h"
#include "../game/scene.h"
#include "player.h"
#include "../game/floor.h"

/**
 * Shot base constructor
 *
 * Calculates the origin of the shot based on the player position.
 * Player uses bottom-middle coordinates (X = center, Y = bottom).
 *
 * @param scn The scene this shot belongs to
 * @param pl The player who fired the shot
 * @param type The weapon type being used
 * @param xOffset Horizontal offset for multi-projectile weapons
 */
Shot::Shot(Scene* scn, Player* pl, WeaponType type, int xOffset)
    : scene(scn), player(pl), weaponType(type), audioChannel(-1)
{
    const WeaponConfig& config = WeaponConfig::get(type);
    weaponSpeed = config.speed;

    // Player X is center; calculate spawn position based on facing direction
    float halfWidth = pl->getWidth() / 2.0f;
    float spriteOffset = pl->getSprite()->getXOff();

    // When facing left, spawn on left side; when facing right, spawn on right side
    if (pl->getFacing() == FacingDirection::LEFT)
    {
        // Convert center to left edge, then apply offsets
        xPos = xInit = pl->getX() - halfWidth + spriteOffset - 2.0f + xOffset;
    }
    else  // FacingDirection::RIGHT
    {
        // Player X is already center; add offset for right-side spawn
        xPos = xInit = pl->getX() + spriteOffset + 5.0f + xOffset;
    }

    // Player Y is bottom (MAX_Y when standing); harpoon spawns from ground level
    yPos = yInit = pl->getY();
}

/**
 * Default ball hit behavior - decrement player shot count and kill
 */
void Shot::onBallHit(Ball* b)
{
    player->looseShoot();
    kill();
}

/**
 * Default floor hit behavior - decrement player shot count and kill
 */
void Shot::onFloorHit(Floor* f)
{
    player->looseShoot();
    kill();
}

/**
 * Default ceiling hit behavior - decrement player shot count and kill
 */
void Shot::onCeilingHit()
{
    player->looseShoot();
    kill();
}

/**
 * Check collision with floor
 * @param fl Floor to check collision with
 * @return true if colliding, false otherwise
 */
bool Shot::collision(Floor* fl)
{
    float cx = xPos + 8;

    if (cx > fl->getX() - 1 && cx < fl->getX() + fl->getWidth() + 1 &&
        fl->getY() + fl->getHeight() > yPos)
    {
        return true;
    }

    return false;
}

/**
 * Called when shot dies - stops associated audio channel
 */
void Shot::onDeath()
{
    if (audioChannel >= 0)
    {
        AudioManager::instance().stopChannel(audioChannel);
        audioChannel = -1;
    }
}
