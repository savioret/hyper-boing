#include "main.h"
#include <cstring>

Player::Player(int sx, int sy, int id)
    : sx(sx), sy(sy), id(id)
{
    xPos = 200.0f + 100.0f * id;
    yPos = (float)(MAX_Y - sy);	
    maxShoots = 2;
    numShoots = 0;
    shotCounter = shotInterval = 20;
    score = 0;

    init();
}

Player::Player(int id)
    : id(id)
{
    init();
}

Player::~Player()
{
}

/**
 * Player initialization
 *
 * Configures the initial sprite, bounding box, movement keys,
 * lives, and score.
 */
void Player::init()
{
    dead = false;
    score = 0;
    frame = 0;
    maxShoots = 2;
    numShoots = 0;
    lives = 3;
    visible = true;
    immuneCounter = 0;
    playing = true;

    shotInterval = 15;
    animSpeed = shotCounter = 10;
    score = 0;
    moveIncrement = 3;
    sprite = &gameinf.getBmp().player[id][ANIM_SHOOT];
    sx = sprite->getWidth();
    sy = sprite->getHeight();
    xPos = 200.0f + 100.0f * id;
    yPos = (float)(MAX_Y - sy);

    xDir = 5;
    yDir = -4;
}

/**
 * Re-initializes the player after death, granting a period
 * of invulnerability.
 */
void Player::revive()
{
    dead = false;
    immuneCounter = 350;

    frame = 0;
    maxShoots = 2;
    numShoots = 0;
    playing = true;

    shotInterval = 15;
    animSpeed = shotCounter = 10;
    sprite = &gameinf.getBmp().player[id][ANIM_SHOOT];
    sx = sprite->getWidth();
    sy = sprite->getHeight();
    
    xPos = 200.0f + 100.0f * id;
    yPos = (float)(MAX_Y - sy);

    xDir = 5;
    yDir = -4;
}

void Player::moveLeft()
{
    if (xPos > MIN_X - 10)
        xPos -= moveIncrement;
    
    if (frame >= ANIM_RIGHT - 1)
    {
        frame = ANIM_LEFT;
        sprite = &gameinf.getBmp().player[id][frame];
        shotCounter = animSpeed;
    }
    else if (!shotCounter)
    {
        if (frame < ANIM_LEFT + 4) frame++;
        else frame = ANIM_RIGHT;
        sprite = &gameinf.getBmp().player[id][frame];
        shotCounter = animSpeed;
    }
    else shotCounter--;
}

void Player::moveRight()
{
    if (xPos + sx < MAX_X - 5)
        xPos += moveIncrement;

    if (frame < ANIM_RIGHT || frame > ANIM_RIGHT + 4)
    {
        frame = ANIM_RIGHT;
        sprite = &gameinf.getBmp().player[id][frame];
        shotCounter = animSpeed;
    }
    else if (!shotCounter)
    {
        if (frame < ANIM_RIGHT + 4) frame++;
        else frame = ANIM_RIGHT;

        sprite = &gameinf.getBmp().player[id][frame];
        shotCounter = animSpeed;
    }
    else shotCounter--;
}

void Player::setFrame(int f)
{
    frame = f;
    sprite = &gameinf.getBmp().player[id][frame];
}

bool Player::canShoot() const
{
    if (numShoots == 0)
    {
        return true;		
    }
    else if (shotCounter == 0 && numShoots < maxShoots)
    {
        return true;
    }
    
    return false;
}

void Player::shoot()
{
    numShoots++;
    shotCounter = shotInterval;
    if (frame != ANIM_SHOOT + 1)
    {
        frame = ANIM_SHOOT + 1;
        sprite = &gameinf.getBmp().player[id][frame];
    }
}

void Player::stop()
{
    if (frame != ANIM_SHOOT)
    {
        if (frame == ANIM_SHOOT + 1)
            if (!shotCounter)
            {
                frame = ANIM_SHOOT;
                sprite = &gameinf.getBmp().player[id][frame];				
                shotCounter = shotInterval;
            }
            else shotCounter--;
        else
        {
            frame = ANIM_SHOOT;
            sprite = &gameinf.getBmp().player[id][frame];
        }
    }
    if (xPos + sx > MAX_X - 10) xPos = (float)(MAX_X - 16 - sx);
}

void Player::update()
{
    if (shotCounter > 0) shotCounter--;
    
    if (dead)
    {
        if (!shotCounter)
        {
            if (frame < ANIM_DEAD + 7) setFrame(frame + 1);
            else setFrame(ANIM_DEAD);
            shotCounter = animSpeed;
        }

        if (xPos + sprite->getWidth() >= MAX_X)
        {
            xDir = -2;
            yDir = 8;
        }
        if (yPos > RES_Y) 
        {		
            if (lives > 0)
            {
                lives--;
                revive();
                return;
            }
            else playing = false;			
        }

        xPos += xDir;
        yPos += yDir;				
    }
    else
    {
        // Handle immunity blinking effect
        if (immuneCounter)
        {
            immuneCounter--;
            visible = !visible;
            if (!immuneCounter) visible = true;
        }
    }
}

void Player::addScore(int num)
{
    score += num;
}

void Player::looseShoot()
{
    if (numShoots > 0)
        numShoots--;
}

/**
 * Handles player death logic.
 */
void Player::kill()
{
    dead = true;
    animSpeed = 4;
}