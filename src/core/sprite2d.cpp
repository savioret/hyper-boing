#include "sprite2d.h"
#include "sprite.h"

Sprite2D::Sprite2D()
    : x(0.0f), y(0.0f), frame(0), visible(true),
      alpha(255.0f), scale(1.0f), rotation(0.0f),
      pivotX(0.5f), pivotY(0.5f), flipH(false), flipV(false)
{
}

void Sprite2D::addSprite(Sprite* sprite)
{
    if (sprite)
    {
        sprites.push_back(sprite);
    }
}

void Sprite2D::clearSprites()
{
    sprites.clear();
    frame = 0;
}

Sprite* Sprite2D::getCurrentSprite() const
{
    if (sprites.empty()) return nullptr;
    if (frame >= 0 && frame < (int)sprites.size())
    {
        return sprites[frame];
    }
    return sprites[0]; // Fallback
}

void Sprite2D::setFrame(int f)
{
    if (f >= 0 && f < (int)sprites.size())
    {
        frame = f;
    }
}

int Sprite2D::getWidth() const
{
    Sprite* s = getCurrentSprite();
    return s ? s->getWidth() : 0;
}

int Sprite2D::getHeight() const
{
    Sprite* s = getCurrentSprite();
    return s ? s->getHeight() : 0;
}
