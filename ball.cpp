#include "pang.h"
#include "math.h"

#define ABSF(x) if((x)<0.0) (x)=-(x);


/********************************************************
  constructor de BALL

  Crea una pelota de identificacion <id>en la posicion <x>, <y> 
  de tamaño size y de altura  maxima de salto <top>, que comenzara 
  desplazandose en la direccion <dirx> <diry>
********************************************************/

BALL::BALL(PSCENE *scn, int _x, int _y, int _size, int _top, int _dirx, int _diry, int _id )
{
    scene = scn;	
    this->x = _x;
    this->y = _y;
    this->size = _size;
    id = _id;

    top = _top;

    dirx = _dirx;
    diry = _diry;
    diameter = scn->bmp.redball[size].sx;

    t = 0;
    y0 = y;



    if(!top) InitTop();
    y = MAX_Y - top;
    Init();
}

/********************************************************
  constructor de BALL

  En este constructor se toma como referencia una pelota
  considerada como la pelota padre, de la que hereda su altura
  y su posicion, y tomando como referencia el tamaño de su padre
  se define el tamaño una unidad mas pequeña.
********************************************************/

BALL::BALL(PSCENE *scn, BALL *oldball)
{
    scene = scn;
    this->x = oldball->x;
    this->y = oldball->y;

    y0 = oldball->y;
    id = oldball->id;
    diry = 1;
    dirx = 1;
    t = oldball->t;

    size = oldball->size + 1; // size = indice del vector de pelotas

    diameter = scn->bmp.redball[size].sx;

    InitTop();
    Init();

    t=0; //t=0 --> v=0
}


BALL::~BALL()
{
}

void BALL::Init()
{		
    hit = FALSE;

    acc = 8/((float)(top-diameter)*(400/top));	
    tmax = sqrt((float)(2*(top-diameter))/(acc));

    if(y0)  // si y0 esta definido
        y0 -= MAX_Y - top; //sacamos la diferencia en pixels

    spr = &scene->bmp.redball[size]; // bitmap correspondiente al tamaño
}

/********************************************************
  InitTop()

  Se llama a esta funcion cuando el parametro top no esta 
  definido(top=0). De esta manera se calcula la altura maxima
  de salto de la pelota en funcion de su diametro..
********************************************************/
void BALL::InitTop()
{

    float d = MAX_Y - MIN_Y;

    if(size==0)
        top = d;
    else if(size==1)
        top = d * 0.6;
    else if(size==2)
        top = d * 0.4;
    else if(size==3)
        top = d * 0.26;
    else
        top = d * 0.1;
}
void BALL::Kill()
{
    hit = TRUE;
}

void BALL::SetDir(int _dirx, int _diry)
{
    dirx = _dirx;
    diry = _diry;	
}

void BALL::SetDirX(int _dirx)
{
    dirx = _dirx;	
}

void BALL::SetDirY(int _diry)
{
    diry = _diry;	
}


/********************************************************
  A continuacion las funciones de colision de la pelota con los
  diferentes elementos del juego.
********************************************************/
BOOL BALL::Colision(SHOOT * sh)
{
    if(!sh->dead)
        if(sh->x > x && sh->x < x+diameter && y+diameter>sh->y && y<sh->yi)	
            return TRUE;		

    return FALSE;
}

POINT BALL::Colision(FLOOR * fl)
{
    POINT col;
    col.x=col.y=0;

    if( (y+diameter > fl->y && y < fl->y + fl->sy) || (y+diameter >= fl->y && y <= fl->y + fl->sy) && (y<= fl->y+fl->sy ||  y+diameter > fl->y + fl->sy) )
        if((x+diameter >= fl->x ) && (x <= fl->x) && dirx == 1) col.x = SIDE_LEFT;
            else if((x <= fl->x + fl->sx) && (x + diameter > fl->x+fl->sx) && dirx == -1) col.x = SIDE_RIGHT;

    if( (x+diameter > fl->x && x < fl->x + fl->sx)	||(x+diameter >= fl->x && x <= fl->x + fl->sx) && (x <= fl->x + fl->sx || x + diameter > fl->x+fl->sx) )
        if((y+diameter >= fl->y) && (y < fl->y) && diry == 1) col.y =  SIDE_TOP;
            else if((y<= fl->y+fl->sy) &&  (y+diameter > fl->y + fl->sy) && diry==-1) col.y =  SIDE_BOTTOM;	

    return col;
}

BOOL BALL::Colision(PLAYER *pl)
{

    if(y+diameter > pl->y+pl->spr->yoff+3)  // lado superior del jugador
        if( x < pl->x + pl->spr->xoff + pl->sx-5 &&  // lado derecho del jugador
            x+diameter > pl->x+pl->spr->xoff+5 )  // lado izquierdo del jugador	
            return TRUE;

    return FALSE;
}

/********************************************************
  Move()

  Mueve la pelota basandose en las ecuaciones fisicas de
  caida libre:
  S=S0 + V0t + 1/2 a * t^2
  tomando <S> como <y> y <S0> como <y0>
********************************************************/

void BALL::Move()
{
    float incy;
    float incx = dirx * 0.80;
    float height = MAX_Y - y;
    static float yprev;
    static float dif=0;		

    if(diry==-1)yprev = y;
    
    y = y0 + 0.5*acc*(t*t);

    y = MAX_Y - top + y;

    if(diry==-1 && y < MAX_Y-diameter-2) dif = y - yprev; 
    else dif = 1000;

    ABSF(dif);

    x += incx;

    t+=diry;

    if(diry==1)	
        if(y+diameter >= MAX_Y)
        {			
            y0=0;
            diry = -1;
            t = tmax; // cuando la pelota llega al suelo adquiere la velocidad calculada
                      // de manera que al subir su altura maxima sea <top>
        }
    if(diry==-1)
    {
        if(dif<0.006)  // cuando la pelota ya apenas sube cambiamos de direccion
        {
            diry = 1;			
        }

        if(t<0) t =0 ;
    }

    if(dirx==1)	
        if(x+diameter>=MAX_X)
        {
            x = MAX_X - diameter;
            dirx = -1;
        }
    if(dirx==-1)
        if(x<=MIN_X)
        {
            x = MIN_X;
            dirx = 1;
        }
}
