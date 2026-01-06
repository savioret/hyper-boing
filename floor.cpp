#include "pang.h"

FLOOR::FLOOR(PSCENE *scn, int _x, int _y, int _id)
{
    scene = scn;
    
    x = _x;
    y = _y;

    

    id = _id;
    spr = &scene->bmp.floor[id];
    
    sx= spr->sx;
    sy= spr->sy;
}

void FLOOR::Update()
{
/*	COLISION col;

    col = Colision();

    if(col.obj)
    {
        if(col.type==OBJ_SHOOT)
            Kill();
    }*/
}

/*COLISION FLOOR::Colision()
{
    MLISTNODE *pt = scene->lsshoots.GetFirstNode();
    SHOOT *sh;
    COLISION col;

    while(pt)
    {
        sh = (SHOOT*) pt->data;
        if(sh->x > x && sh->x < x+sx && y+sy>sh->y)	
        {
            sh->Kill();
            col.obj = OBJ_SHOOT;
            return col;
        }
        pt = scene->lsshoots.GetNextNode(pt);
    }

    col.obj=0;
    return col;
}*/