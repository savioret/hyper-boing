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
Ball::Ball(Scene* scn, int x, int y, int size, float dx, int dy, int topVal, int idVal)
{
    scene = scn;
    this->xPos = (float)x;
    this->yPos = (float)y;  // Absolute screen coordinate
    this->size = size;
    id = idVal;

    top = topVal;

    dirX = dx;
    dirY = dy;

    // Get diameter from the ball spritesheet
    StageResources& res = gameinf.getStageRes();
    if (res.ballAnim)
    {
        Sprite* spr = res.ballAnim->getFrame(size);
        diameter = spr ? spr->getWidth() : 64;  // Fallback to 64 if sprite not found
    }
    else
    {
        diameter = 64;  // Fallback
    }

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

    // Get diameter from the ball spritesheet
    StageResources& res = gameinf.getStageRes();
    if (res.ballAnim)
    {
        Sprite* spr = res.ballAnim->getFrame(size);
        diameter = spr ? spr->getWidth() : 40;  // Fallback
    }
    else
    {
        diameter = 40;  // Fallback
    }

    // Calculate center of parent ball
    float parentCenterX = oldball->xPos + (oldball->diameter / 2.0f);
    float parentCenterY = oldball->yPos + (oldball->diameter / 2.0f);

    // Position new ball at parent's center (top-left positioning)
    this->xPos = parentCenterX - (diameter / 2.0f);
    this->yPos = parentCenterY - (diameter / 2.0f);

    // Apply directional offset proportional to diameter
    float offset = diameter * 0.5f;
    this->xPos += offset * dirX;

    //y0 = oldball->yPos;

    initTop();
    init();

    // Initialize for upward push from current position
    dirY = -1;
    time = maxTime / 2;  // Start at mid-flight to move upwards

    // Calculate y0 so physics equation gives us yPos at this time:
    // yPos = (MAX_Y - top) + y0 + 0.5 * gravity * time²
    // Solving: y0 = yPos - (MAX_Y - top) - 0.5 * gravity * time²
    y0 = yPos - (float)(Stage::MAX_Y - top) - 0.5f * gravity * time * time;
}

Ball::~Ball()
{
}

void Ball::init()
{
    gravity = 8 / ((float)(top - diameter) * (400.0f / top));
    maxTime = std::sqrt((float)(2 * (top - diameter)) / (gravity));
}

Sprite* Ball::getCurrentSprite() const
{
    StageResources& res = gameinf.getStageRes();
    if (res.ballAnim)
    {
        return res.ballAnim->getFrame(size);
    }
    return nullptr;
}

void Ball::kill()
{
    if (!flashing && !dead)
    {
        flashing = true;
        flashTimer = FLASH_DURATION;
        // onDeath() is called by IGameObject::kill() when the flash expires,
        // ensuring it fires exactly once (splash effect, events, pickup spawn).
    }
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

void Ball::setDir(float dx, int dy)
{
    dirX = dx;
    dirY = dy;
}

void Ball::setDirX(float dx)
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

bool Ball::collision(Platform* fl)
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
    // Handle flash state (countdown before actual death)
    if (flashing)
    {
        flashTimer -= dt;
        if (flashTimer <= 0.0f)
        {
            // Flash complete - now actually die
            IGameObject::kill();  // Call parent to set dead=true
        }
        return;  // Don't move during flash
    }

    float incx = dirX * 0.80f;

    // Calculate new Y position using physics equation
    yPos = y0 + 0.5f * gravity * (time * time);
    yPos = (float)(Stage::MAX_Y - top) + yPos;

    xPos += incx;
    time += (float)dirY;

    if (dirY == 1)
    {
        // Falling: check if hit floor
        if (yPos + diameter >= Stage::MAX_Y)
        {
            y0 = 0;
            dirY = -1;
            time = maxTime;
        }
    }
    else if (dirY == -1)
    {
        // Rising: check if hit ceiling
        if (yPos <= Stage::MIN_Y)
        {
            yPos = (float)Stage::MIN_Y;
            dirY = 1;
            time = 0;
            // Calculate y0 so ball falls from ceiling position
            y0 = (float)Stage::MIN_Y - (float)(Stage::MAX_Y - top);
        }
        // Rising: check if reached peak (time-based detection)
        else if (time <= 0)
        {
            time = 0;
            dirY = 1;
        }
    }

    if (dirX > 0)
    {
        if (xPos + diameter >= Stage::MAX_X)
        {
            xPos = (float)(Stage::MAX_X - diameter);
            dirX = -dirX;
        }
    }
    else if (dirX < 0)
    {
        if (xPos <= Stage::MIN_X)
        {
            xPos = (float)Stage::MIN_X;
            dirX = -dirX;
        }
    }
}

void Ball::onDeath()
{
    // Spawn pop animation centered on this ball with size-based scaling
    // Size 0 = 100%, Size 1 = 50%, Size 2 = 33%, Size 3 = 25%
    static const float sizeScales[] = { 1.0f, 0.5f, 0.33f, 0.25f };
    float scale = (size >= 0 && size <= 3) ? sizeScales[size] : 0.25f;

    StageResources& res = gameinf.getStageRes();
    AnimSpriteSheet* tmpl = res.ballSplashAnim.get();
    if (tmpl)
    {
        int cx = (int)xPos + diameter / 2;
        int cy = (int)yPos + diameter / 2 + (int)(tmpl->getHeight() * scale);
        scene->spawnEffect(tmpl, cx, cy, scale);
    }

    // Fire BALL_SPLIT event if ball will split into smaller balls
    if (size < 3)
    {
        GameEventData event(GameEventType::BALL_SPLIT);
        event.ballSplit.parentBall = this;
        event.ballSplit.parentSize = size;
        EVENT_MGR.trigger(event);
    }

    // Spawn only the death pickup whose size matches this ball's current size.
    for (int i = 0; i < deathPickupCount; i++)
    {
        if (deathPickups[i].size == size)
        {
            int cx = (int)xPos + diameter / 2;
            int cy = (int)yPos + diameter / 2;
            scene->addPickup(cx, cy, deathPickups[i].type);
            break;  // At most one pickup per size level
        }
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

    // Propagate pickups bound to sizes beyond this ball to one random child.
    // Both children are size+1; only one (chosen randomly) inherits the entries
    // so each pickup item can only appear once across the whole split tree.
    Ball* lucky = (rand() % 2 == 0) ? child1.get() : child2.get();
    for (int i = 0; i < deathPickupCount; i++)
    {
        if (deathPickups[i].size > size)
            lucky->addDeathPickup(deathPickups[i]);
    }

    return { std::move(child1), std::move(child2) };
}