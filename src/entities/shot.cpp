#include "main.h"
#include "shot.h"
#include "../game/scene.h"
#include "player.h"
#include "../game/floor.h"

/**
 * Shot base constructor
 *
 * Calculates the origin of the shot based on the center of the player
 * who fired it, adjusted for their sprite's current offset.
 *
 * @param scn The scene this shot belongs to
 * @param pl The player who fired the shot
 * @param type The weapon type being used
 * @param xOffset Horizontal offset for multi-projectile weapons
 */
Shot::Shot(Scene* scn, Player* pl, WeaponType type, int xOffset)
    : scene(scn), player(pl), weaponType(type)
{
    const WeaponConfig& config = WeaponConfig::get(type);
    weaponSpeed = config.speed;

    // Apply horizontal offset for multi-projectile weapons
    xPos = xInit = (pl->getX() + pl->getWidth() / 2.0f) + pl->getSprite()->getXOff() + 5.0f + xOffset;
    yPos = yInit = (float)MAX_Y;
}

/**
 * Default ball hit behavior - just kill the shot
 */
void Shot::onBallHit(Ball* b)
{
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
