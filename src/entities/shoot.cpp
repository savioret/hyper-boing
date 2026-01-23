#include "main.h"
#include "shoot.h"

/**
 * Shoot constructor
 *
 * It calculates the origin of the shot based on the center of the player
 * who fired it, adjusted for their sprite's current offset.
 * @param scn The scene this shoot belongs to
 * @param pl The player who fired the shoot
 * @param type The weapon type being used
 * @param xOffset Horizontal offset for multi-projectile weapons
 */
Shoot::Shoot(Scene* scn, Player* pl, WeaponType type, int xOffset)
    : scene(scn), player(pl), weaponType(type)
{
    const WeaponConfig& config = WeaponConfig::get(type);
    weaponSpeed = config.speed;

    // Apply horizontal offset for multi-projectile weapons
    xPos = xInit = (pl->getX() + pl->sx / 2.0f) + pl->getSprite()->getXOff() + 5.0f + xOffset;
    yPos = yInit = (float)MAX_Y;
    id = static_cast<int>(type);

    tail = 0;
    tailTime = shotCounter = 2;

    // Use different sprite based on weapon type (for now, use same sprites)
    sprites[0] = &scene->bmp.shoot[0];
    sprites[1] = &scene->bmp.shoot[1];
    sprites[2] = &scene->bmp.shoot[2];
}

Shoot::~Shoot()
{
}

bool Shoot::collision(Floor* fl)
{
    float cx = xPos + 8;

    if (cx > fl->getX() - 1 && cx < fl->getX() + fl->getWidth() + 1 && fl->getY() + fl->getHeight() > yPos)	
        return true;

    return false;
}

void Shoot::move()
{
    if (!shotCounter)
    {
        tail = !tail;
        shotCounter = tailTime;
    }
    else shotCounter--;

    if (!isDead())
    {
        if (yPos <= MIN_Y)
        {
            player->looseShoot();
            kill();
        }
        else yPos -= weaponSpeed;  // Use weapon-specific speed
    }
}

