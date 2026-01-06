#include "pang.h"

PLAYER::PLAYER(int _sx, int _sy, int _id)
{
    sx = _sx;
    sy = _sy;

    id = _id;

    x = 200+ 100*id;
    y = MAX_Y-sy;	
    maxshoots = 2;
    numshoots = 0;
    cont = interval = 20;
    score = 0;


    Init();
}

PLAYER::PLAYER(int _id)
{
    id = _id;	
    
    Init();
}

PLAYER::~PLAYER()
{

}

void PLAYER::Init()
{
    dead = FALSE;
    score = 0;
    frame = 0;
    maxshoots = 2;
    numshoots = 0;
    lives = 3;
    visible = TRUE;
    immune = 0;
    playing = TRUE;

    interval = 15;
    speed = cont = 10;
    score = 0;
    inc = 3;
    spr = &gameinf.bmp.player[id][ANIM_SHOOT];
    sx = spr->sx;
    sy = spr->sy;
    x = 200+ 100*id;
    y = MAX_Y - sy;

    xdir = 5;
    ydir = -4;
}

/*******************************************************
    Revive()

    Resucita al personaje.
    Restaura sus variables, pero mantiene su puntuacion y
    el numero de vidas, que supuestamente ha sido decrementado 
    previamente.
    Asimismo, activa la variable immuno, que hace inmune al
    jugador durante 6 segundos aproximadamente.

    * Esta funcion es llamada cuando perdemos una vida.
*********************************************************/
void PLAYER::Revive()
{
    dead = FALSE;
    immune = 350;

    frame = 0;
    maxshoots = 2;
    numshoots = 0;
    playing = TRUE;

    interval = 15;
    speed = cont = 10;
    spr = &gameinf.bmp.player[id][ANIM_SHOOT];
    sx = spr->sx;
    sy = spr->sy;
    
    x = 200+ 100*id;
    y = MAX_Y - sy;

    xdir = 5;
    ydir = -4;
}

void PLAYER::MoveLeft()
{
    if(x>MIN_X-10)
        x-=inc;
    
        
    if(frame >= ANIM_RIGHT-1)
    {
        frame = ANIM_LEFT;
        spr = &gameinf.bmp.player[id][frame];
        cont = speed;
    }
    else	
    if(!cont)
    {
        if(frame<ANIM_LEFT+4)frame++;
            else frame = ANIM_RIGHT;
        spr = &gameinf.bmp.player[id][frame];
        cont = speed;
    }else cont--;
}

void PLAYER::MoveRight()
{
    if(x+sx<MAX_X-5)
        x+=inc;

    // si estaba andando hacia la izq cambiamos		
    if(frame < ANIM_RIGHT ||frame > ANIM_RIGHT + 4)
    {
        frame = ANIM_RIGHT;
        spr = &gameinf.bmp.player[id][frame];
        cont = speed;
    }
    else //sino pues pasamos al siguiente frame
    if(!cont)
    {
        if(frame<ANIM_RIGHT+4)frame++;
            else frame = ANIM_RIGHT;

        spr = &gameinf.bmp.player[id][frame];
        cont = speed;
    }else cont--;
}

/*******************************************************
    SetFrame()

    Pone el bitmap actual del personaje dependiendo del frame
    que indiquemos.
*********************************************************/
void PLAYER::SetFrame(int _frame)
{
    frame = _frame;
    spr = &gameinf.bmp.player[id][frame];
}

BOOL PLAYER::CanShoot()
{
    int disp = FALSE;
    // si no hay ningun disparo entonces disparamos
    /* PERO si hay algun disparo comprobamoss si se ha superado
       el tiempo de intervalo, y que el numero de disparos
       en pantalla sea el correcto
    */
    
    if(!numshoots)
    {
        disp = TRUE;		
    }
    else
        if(!cont && numshoots<maxshoots)
            disp = TRUE;
    
    return disp;
}

/*******************************************************
    Shoot()

    Añade un disparo al contador de disparos y resetea el contador
    de tiempo de manera que no podamos disparar dos veces en 
    un periodo de tiempo.
    Tambien actualiza el bitmap correspondiente al disparo.
*********************************************************/
void PLAYER::Shoot()
{
    numshoots++;
    cont = interval;
    if(frame != ANIM_SHOOT+1)
    {
        frame = ANIM_SHOOT+1;
        spr = &gameinf.bmp.player[id][frame];
    }
}

/*******************************************************
    Stop()

    Coloca el jugador en posicion parada. Preparado para
    disparar.
    Del mismo modo decrementa los contadores que permiten
    disparar o no en un pequeño intervalo de tiempo.
*********************************************************/
void PLAYER::Stop()
{
    if(frame != ANIM_SHOOT)
    {
        if(frame==ANIM_SHOOT+1)
            if(!cont)
            {
                frame = ANIM_SHOOT;
                spr = &gameinf.bmp.player[id][frame];				
                cont = interval;
            }else cont --;
        else
        {
            frame = ANIM_SHOOT;
            spr = &gameinf.bmp.player[id][frame];
        }
    }
    if(x+sx > MAX_X-10) x = MAX_X - 16 - sx;
}

/*******************************************************
    Update()

    Actualiza el personaje. Actualiza sus contadores y sus
    variables de estado.
*********************************************************/
void PLAYER::Update()
{

    if(cont>0) cont--;
    
    if(dead) /* animacion de muerto */
    {
        if(!cont)
        {
            if(frame<ANIM_DEAD+7) SetFrame(frame+1);
            else SetFrame(ANIM_DEAD);
            cont = speed;
        }

        if(x+spr->sx>=MAX_X)
        {
            xdir = -2;
            ydir = 8;
        }
        if(y>RES_Y) 
        {		
            if(lives>0)
            {
                lives = lives - 1;
                Revive();
                return;
            }
            else playing = FALSE;			
        }

        x+= xdir;
        y+= ydir;				
    }
    else
    {
        if(immune)
        {
            immune--;
            visible = !visible;
            if(!immune) visible = TRUE;
        }
    }
}


void PLAYER::AddScore(int num)
{
    score +=num;

}


/************************************************************
            LooseShoot()					
                                                
    Con esta funcion se hace saber al Player		
    que uno de los disparos que ha lanzado ya	
    ha dejado de existir							
*************************************************************/
void PLAYER::LooseShoot()
{
    if(numshoots)
        numshoots--;
}


void PLAYER::Kill()
{
    dead = TRUE;
    speed = 4;
}