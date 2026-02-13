#include "main.h"

Floor::Floor(Scene* scene, int x, int y, int id)
    : scene(scene), id(id)
{
    sprite.setSprite(&gameinf.getStageRes().floor[id]);
    this->xPos = (float)x;
    this->yPos = (float)y;
    sx = sprite.getWidth();
    sy = sprite.getHeight();
}

Floor::~Floor()
{
}

void Floor::update(float dt)
{
    // Currently floors are static, but dt is available for future animations
}