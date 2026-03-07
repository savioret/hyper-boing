#include "main.h"

Floor::Floor(Scene* scene, int x, int y, FloorType type)
    : scene(scene), type(type)
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
    return gameinf.getStageRes().floorBricksAnim->getFrame(static_cast<int>(type));
}
