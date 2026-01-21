#include "main.h"
#include "shoot.h"

/**
 * Shoot constructor
 *
 * It calculates the origin of the shot based on the center of the player
 * who fired it, adjusted for their sprite's current offset.
 */
Shoot::Shoot(Scene* scn, Player* pl)
    : scene(scn), player(pl)
{	
    xPos = xInit = (pl->getX() + pl->sx / 2.0f) + pl->getSprite()->getXOff() + 5.0f;
    yPos = yInit = (float)MAX_Y;
    id = pl->idWeapon;

    speed = 5;
    tail = 0;

    tailTime = shotCounter = 2;

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
        else yPos -= speed;
    }
}

