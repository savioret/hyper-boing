#include "main.h"
#include "animspritesheet.h"
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
    this->yPos = (float)y;  // Absolute screen coordinate
    this->size = size;
    id = idVal;

    top = topVal;

    dirX = dx;
    dirY = dy;
    diameter = gameinf.getStageRes().redball[size].getWidth();

    time = 0;

    if (!top)
        initTop();

    // Calculate y0 as offset from baseline for physics
    // (baseline = Stage::MAX_Y - top = peak bounce height)
    y0 = yPos - (float)(Stage::MAX_Y - top);

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
 * The new ball is created at the same center as the parent ball, but with its
 * size reduced by one unit. A directional offset is applied based on the dir parameter
 * to separate the two child balls.
 */
Ball::Ball(Scene* scn, Ball* oldball, int dir)
{
    scene = scn;
    id = oldball->id;
    dirY = 1;
    dirX = dir;
    time = oldball->time;

    size = oldball->size + 1;

    diameter = gameinf.getStageRes().redball[size].getWidth();

    // Calculate center of parent ball
    float parentCenterX = oldball->xPos + (oldball->diameter / 2.0f);
    float parentCenterY = oldball->yPos + (oldball->diameter / 2.0f);

    // Position new ball at parent's center (top-left positioning)
    this->xPos = parentCenterX - (diameter / 2.0f);
    this->yPos = parentCenterY - (diameter / 2.0f);

    // Apply directional offset proportional to diameter
    float offset = diameter * 0.5f;
    this->xPos += offset * dirX;

    y0 = oldball->yPos;

    initTop();
    init();
    
    y0 = yPos - ( float )(Stage::MAX_Y - top);
    time = 0;
}

Ball::~Ball()
{
}

void Ball::init()
{
    gravity = 8 / ((float)(top - diameter) * (400.0f / top));
    maxTime = std::sqrt((float)(2 * (top - diameter)) / (gravity));

    sprite.setSprite(&gameinf.getStageRes().redball[size]);
}

/**
 * initTop()
 *
 * This function is called when the top parameter is not defined (top=0).
 * It calculates the maximum jump height for the ball based on its diameter.
 */
void Ball::initTop()
{
    float d = (float)(Stage::MAX_Y - Stage::MIN_Y);

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
    if (sh->isDead())
        return false;

    // Use AABB collision detection with collision boxes
    return intersects(getCollisionBox(), sh->getCollisionBox());
}

bool Ball::collision(Floor* fl)
{
    return intersects(getCollisionBox(), fl->getCollisionBox());
}

bool Ball::collision(Player* pl)
{
    // Use AABB collision detection with collision boxes
    return intersects(getCollisionBox(), pl->getCollisionBox());
}

/**
 * move()
 *
 * Moves the ball based on the physical equations of free fall:
 * S = S0 + V0t + 1/2 a * t^2
 * where S is y and S0 is y0.
 */
void Ball::update(float dt)
{
    float incx = dirX * 0.80f;
    static float yPrev;
    static float dif = 0;		

    if (dirY == -1) yPrev = yPos;
    
    yPos = y0 + 0.5f * gravity * (time * time);
    yPos = (float)(Stage::MAX_Y - top) + yPos;

    if (dirY == -1 && yPos < Stage::MAX_Y - diameter - 2) 
        dif = yPos - yPrev; 
    else 
        dif = 1000.0f;

    float absDif = std::abs(dif);

    xPos += incx;
    time += (float)dirY;

    if (dirY == 1)	
    {
        if (yPos + diameter >= Stage::MAX_Y)
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
        if (xPos + diameter >= Stage::MAX_X)
        {
            xPos = (float)(Stage::MAX_X - diameter);
            dirX = -1;
        }
    }
    else if (dirX == -1)
    {
        if (xPos <= Stage::MIN_X)
        {
            xPos = (float)Stage::MIN_X;
            dirX = 1;
        }
    }
}

void Ball::onDeath()
{
    // Spawn pop animation centered on this ball
    StageResources& res = gameinf.getStageRes();
    int popIndex = (size == 0) ? 0 : (size == 1) ? 1 : 2;
    AnimSpriteSheet* tmpl = res.ballPopAnim[popIndex].get();
    if (tmpl)
        scene->spawnEffect(tmpl, (int)xPos + diameter / 2, (int)yPos + diameter / 2 + tmpl->getHeight());

    // Fire BALL_SPLIT event if ball will split into smaller balls
    if (size < 3)
    {
        GameEventData event(GameEventType::BALL_SPLIT);
        event.ballSplit.parentBall = this;
        event.ballSplit.parentSize = size;
        EVENT_MGR.trigger(event);
    }
}

std::pair<std::unique_ptr<Ball>, std::unique_ptr<Ball>> Ball::createChildren()
{
    // Only create children if ball is large enough to split
    if (size >= 3)
    {
        return { nullptr, nullptr };
    }

    auto child1 = std::make_unique<Ball>(scene, this, -1);  // Left
    auto child2 = std::make_unique<Ball>(scene, this, 1);   // Right
    
    return { std::move(child1), std::move(child2) };
}