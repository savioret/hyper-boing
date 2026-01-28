#include "main.h"

Floor::Floor(Scene* scene, int x, int y, int id)
    : scene(scene), id(id)
{
    sprite = &gameinf.getStageRes().floor[id];
    this->xPos = (float)x;
    this->yPos = (float)y;
    sx = sprite->sx;
    sy = sprite->sy;
}

Floor::~Floor()
{
}

void Floor::update(float dt)
{
    // Currently floors are static, but dt is available for future animations
}