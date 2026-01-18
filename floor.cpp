#include "pang.h"

Floor::Floor(Scene* scene, int x, int y, int id)
    : scene(scene), x(x), y(y), id(id)
{
    sprite = &scene->bmp.floor[id];

    sx = sprite->sx;
    sy = sprite->sy;
}

Floor::~Floor()
{
}

void Floor::update()
{
    /*	COLISION col;

        col = Colision();

        if(col.obj)
        {
            if(col.type==OBJ_SHOOT)
                kill();
        }*/
}