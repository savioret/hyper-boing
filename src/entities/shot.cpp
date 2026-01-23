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

    // Calculate shot spawn position based on player facing direction
    float centerOffset = pl->getWidth() / 2.0f;
    float spriteOffset = pl->getSprite()->getXOff();

    // When facing left, spawn on left side; when facing right, spawn on right side
    if (pl->getFacing() == FacingDirection::LEFT)
    {
        xPos = xInit = pl->getX() + spriteOffset - 2.0f + xOffset;
    }
    else  // FacingDirection::RIGHT
    {
        xPos = xInit = pl->getX() + centerOffset + spriteOffset + 5.0f + xOffset;
    }

    yPos = yInit = ( float )MAX_Y;
    //yPos = yInit =  pl->getY() + 2.0f;  // Spawn slightly above player's Y position
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
