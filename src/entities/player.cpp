#include "main.h"
#include "eventmanager.h"
#include <cstring>
#include <SDL.h>

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
    currentWeapon = WeaponType::HARPOON;  // Default weapon
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
    facing = FacingDirection::RIGHT;  // Default facing right
    sprite = &gameinf.getBmp().player[id][ANIM_SHOOT];
    xPos = 200.0f + 100.0f * id;
    yPos = (float)(MAX_Y - sprite->getHeight());

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
    currentWeapon = WeaponType::HARPOON;  // Reset to default on death
    maxShoots = 2;
    numShoots = 0;
    playing = true;

    shotInterval = 15;
    animSpeed = shotCounter = 10;
    facing = FacingDirection::RIGHT;  // Reset facing on revive
    sprite = &gameinf.getBmp().player[id][ANIM_SHOOT];

    xPos = 200.0f + 100.0f * id;
    yPos = (float)(MAX_Y - sprite->getHeight());

    xDir = 5;
    yDir = -4;
}

void Player::moveLeft()
{
    facing = FacingDirection::LEFT;  // Update facing direction

    if (xPos > MIN_X - 10)
        xPos -= moveIncrement;

    if (frame >= ANIM_SHOOT - 1)
    {
        frame = ANIM_WALK;
        sprite = &gameinf.getBmp().player[id][frame];
        shotCounter = animSpeed;
    }
    else if (!shotCounter)
    {
        if (frame < ANIM_WALK + 4) frame++;
        else frame = ANIM_WALK;
        sprite = &gameinf.getBmp().player[id][frame];
        shotCounter = animSpeed;
    }
    else shotCounter--;
}

void Player::moveRight()
{
    facing = FacingDirection::RIGHT;  // Update facing direction

    if (xPos + sprite->getWidth() < MAX_X - 5)
        xPos += moveIncrement;

    if (frame >= ANIM_SHOOT - 1)
    {
        frame = ANIM_WALK;
        sprite = &gameinf.getBmp().player[id][frame];
        shotCounter = animSpeed;
    }
    else if (!shotCounter)
    {
        if (frame < ANIM_WALK + 4) frame++;
        else frame = ANIM_WALK;
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
    if (xPos + sprite->getWidth() > MAX_X - 10) xPos = (float)(MAX_X - 16 - sprite->getWidth());
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
    int previousScore = score;
    score += num;

    // Fire SCORE_CHANGED event
    GameEventData event;
    event.type = GameEventType::SCORE_CHANGED;
    event.timestamp = SDL_GetTicks();
    event.scoreChanged.player = this;
    event.scoreChanged.scoreAdded = num;
    event.scoreChanged.previousScore = previousScore;
    event.scoreChanged.newScore = score;
    EVENT_MGR.trigger(event);
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

/**
 * Sets the player's weapon type and updates weapon-specific parameters.
 * @param type The weapon type to switch to
 */
void Player::setWeapon(WeaponType type)
{
    currentWeapon = type;
    const WeaponConfig& config = WeaponConfig::get(type);
    maxShoots = config.maxShots;
    shotInterval = config.cooldown;
}