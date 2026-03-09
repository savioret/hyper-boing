#include "main.h"

Floor::Floor(Scene* scene, int x, int y, FloorType type, int colorVal)
    : scene(scene), type(type), color(static_cast<Color>(colorVal))
{
    this->xPos = (float)x;
    this->yPos = (float)y;
    Sprite* spr = getCurrentSprite();
    sx = spr ? spr->getWidth()  : 0;
    sy = spr ? spr->getHeight() : 0;
}

Floor::~Floor()
{
}

void Floor::update(float dt)
{
    // Currently floors are static, but dt is available for future animations
}

Sprite* Floor::getCurrentSprite() const
{
    AnimSpriteSheet* anim = gameinf.getStageRes().getFloorAnim(static_cast<int>(color));
    return anim ? anim->getFrame(static_cast<int>(type)) : nullptr;
}
