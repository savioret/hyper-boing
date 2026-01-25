#include "main.h"
#include "eventmanager.h"
#include <cmath>
#include <SDL.h>

#define ABSF(x) if((x)<0.0) (x)=-(x);

/********************************************************
  constructor de BALL

  Crea una pelota de identificacion <id>en la posicion <x>, <y> 
  de tamaño size y de altura  maxima de salto <top>, que comenzara 
  desplazandose en la direccion <dirx> <diry>
********************************************************/

/**
 * BALL constructor
 *
 * When a ball is created, its size and position (x, y) are passed.
 * The ball starts either moving up or down (dirY) and moving in the
 * horizontal direction (dirX).
 */
Ball::Ball(Scene* scn, int x, int y, int size, int dx, int dy, int topVal, int idVal)
{
    scene = scn;	
    this->xPos = (float)x;
    this->yPos = (float)y;
    this->size = size;
    id = idVal;

    top = topVal;

    dirX = dx;
    dirY = dy;
    diameter = scn->bmp.redball[size].getWidth();

    time = 0;
    y0 = yPos;

    if (!top) initTop();
    yPos = (float)(MAX_Y - top);
    init();
}

/********************************************************
  constructor de BALL

  En este constructor se toma como referencia una pelota
  considerada como la pelota padre, de la que hereda su altura
  y su posicion, y tomando como referencia el tamaño de su padre
  se define el tamaño una unidad mas pequeña.
********************************************************/

/**
 * BALL constructor
 *
 * Creates a new ball from an existing one. This occurs when a ball is hit by a shot.
 * The new ball is created at the same position as the destroyed one, but with its
 * size reduced by one unit.
 */
Ball::Ball(Scene* scn, Ball* oldball)
{
    scene = scn;
    this->xPos = oldball->xPos;
    this->yPos = oldball->yPos;

    y0 = oldball->yPos;
    id = oldball->id;
    dirY = 1;
    dirX = 1;
    time = oldball->time;

    size = oldball->size + 1;

    diameter = scn->bmp.redball[size].getWidth();

    initTop();
    init();

    time = 0;
}

Ball::~Ball()
{
}

void Ball::init()
{		
    gravity = 8 / ((float)(top - diameter) * (400.0f / top));	
    maxTime = std::sqrt((float)(2 * (top - diameter)) / (gravity));

    if (y0 != 0)
        y0 -= (float)(MAX_Y - top);

    sprite = &scene->bmp.redball[size];
}

/**
 * initTop()
 *
 * This function is called when the top parameter is not defined (top=0).
 * It calculates the maximum jump height for the ball based on its diameter.
 */
void Ball::initTop()
{
    float d = (float)(MAX_Y - MIN_Y);

    if (size == 0)
        top = (int)d;
    else if (size == 1)
        top = (int)(d * 0.6f);
    else if (size == 2)
        top = (int)(d * 0.4f);
    else if (size == 3)
        top = (int)(d * 0.26f);
    else
        top = (int)(d * 0.1f);
}

void Ball::setDir(int dx, int dy)
{
    dirX = dx;
    dirY = dy;	
}

void Ball::setDirX(int dx)
{
    dirX = dx;	
}

void Ball::setDirY(int dy)
{
    dirY = dy;	
}

void Ball::setPos(int x, int y)
{
    xPos = (float)x;
    yPos = (float)y;
}

/**
 * Collision detection functions for the ball with various game elements.
 */
bool Ball::collision(Shot* sh)
{
    if (!sh->isDead())
        if (sh->getX() > xPos && sh->getX() < xPos + diameter && yPos + diameter > sh->getY() && yPos < sh->getYInit())
            return true;

    return false;
}

SDL_Point Ball::collision(Floor* fl)
{
    SDL_Point col = { 0, 0 };

    if ((yPos + diameter > fl->getY() && yPos < fl->getY() + fl->getHeight()) || 
        ((yPos + diameter >= fl->getY() && yPos <= fl->getY() + fl->getHeight()) && 
         (yPos <= fl->getY() + fl->getHeight() || yPos + diameter > fl->getY() + fl->getHeight())))
    {
        if ((xPos + diameter >= fl->getX()) && (xPos <= fl->getX()) && dirX == 1) 
            col.x = SIDE_LEFT;
        else if ((xPos <= fl->getX() + fl->getWidth()) && (xPos + diameter > fl->getX() + fl->getWidth()) && dirX == -1) 
            col.x = SIDE_RIGHT;
    }

    if ((xPos + diameter > fl->getX() && xPos < fl->getX() + fl->getWidth()) || 
        ((xPos + diameter >= fl->getX() && xPos <= fl->getX() + fl->getWidth()) && 
         (xPos <= fl->getX() + fl->getWidth() || xPos + diameter > fl->getX() + fl->getWidth())))
    {
        if ((yPos + diameter >= fl->getY()) && (yPos < fl->getY()) && dirY == 1) 
            col.y = SIDE_TOP;
        else if ((yPos <= fl->getY() + fl->getHeight()) && (yPos + diameter > fl->getY() + fl->getHeight()) && dirY == -1) 
            col.y = SIDE_BOTTOM;	
    }

    return col;
}

bool Ball::collision(Player* pl)
{
    if (yPos + diameter > pl->getY() + pl->getSprite()->getYOff() + 3)
    {
        if (xPos < pl->getX() + pl->getSprite()->getXOff() + pl->getWidth() - 5 && 
            xPos + diameter > pl->getX() + pl->getSprite()->getXOff() + 5)
        {
            return true;
        }
    }

    return false;
}

/**
 * move()
 *
 * Moves the ball based on the physical equations of free fall:
 * S = S0 + V0t + 1/2 a * t^2
 * where S is y and S0 is y0.
 */
void Ball::move()
{
    float incx = dirX * 0.80f;
    static float yPrev;
    static float dif = 0;		

    if (dirY == -1) yPrev = yPos;
    
    yPos = y0 + 0.5f * gravity * (time * time);
    yPos = (float)(MAX_Y - top) + yPos;

    if (dirY == -1 && yPos < MAX_Y - diameter - 2) 
        dif = yPos - yPrev; 
    else 
        dif = 1000.0f;

    float absDif = std::abs(dif);

    xPos += incx;
    time += (float)dirY;

    if (dirY == 1)	
    {
        if (yPos + diameter >= MAX_Y)
        {			
            y0 = 0;
            dirY = -1;
            time = maxTime;
        }
    }
    else if (dirY == -1)
    {
        if (absDif < 0.006f)
        {
            dirY = 1;			
        }

        if (time < 0) time = 0;
    }

    if (dirX == 1)	
    {
        if (xPos + diameter >= MAX_X)
        {
            xPos = (float)(MAX_X - diameter);
            dirX = -1;
        }
    }
    else if (dirX == -1)
    {
        if (xPos <= MIN_X)
        {
            xPos = (float)MIN_X;
            dirX = 1;
        }
    }
}

void Ball::onDeath()
{
    // Fire BALL_SPLIT event if ball will split into smaller balls
    if (size < 3)
    {
        GameEventData event;
        event.type = GameEventType::BALL_SPLIT;
        event.timestamp = SDL_GetTicks();
        event.ballSplit.parentBall = this;
        event.ballSplit.parentSize = size;
        EVENT_MGR.trigger(event);
    }

    // Queue this ball to split into smaller balls
    scene->splitBall(this);
}