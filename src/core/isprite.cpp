#include "isprite.h"
#include "sprite.h"

int ISprite::getWidth() const
{
    Sprite* spr = getActiveSprite();
    return spr ? spr->getWidth() : 0;
}

int ISprite::getHeight() const
{
    Sprite* spr = getActiveSprite();
    return spr ? spr->getHeight() : 0;
}
