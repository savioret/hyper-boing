#include "pang.h"


SHOOT::SHOOT(PSCENE *scn, PLAYER *pl)
{	
    scene = scn;
    player = pl;

    x = xi = (pl->x+pl->sx/2) + pl->spr->xoff + 5;
    y = yi = MAX_Y;
    id = pl->idweapon;

    speed = 5;
    dead = FALSE;
    tail = 0;

    tailtime = cont = 2;

    spr[0] = &scene->bmp.shoot[0];
    spr[1] = &scene->bmp.shoot[1];
    spr[2] = &scene->bmp.shoot[2];
}


SHOOT::~SHOOT()
{
}

BOOL SHOOT::Colision(FLOOR *fl)
{
    int cx =  x +8;

    if(cx > fl->x-1 && cx < fl->x+fl->sx+1 && fl->y+fl->sy>y)	
        return TRUE;

    return FALSE;

}

void SHOOT::Move()
{	

    if(!cont)
    {
        tail = !tail;
        cont = tailtime;
    }else cont--;

    if(!dead)
    {
        if (y <= MIN_Y)
        {

            /*if (yi >= y)
                yi-=speed;
            else*/
            player->LooseShoot();
            dead = 1;
        }
        else y-=speed;
    }

/*	col = Colision();
    if(col.obj)
        switch(col.obj)
        {
            case OBJ_BALL: dead = 1; break;
        }*/
}

