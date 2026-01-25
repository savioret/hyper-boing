#include "main.h"
#include "eventmanager.h"
#include "playerdeadaction.h"
#include <cstring>
#include <SDL.h>

Player::Player(int id)
    : id(id), deathAction(nullptr)
{
    init();

    // Subscribe to PLAYER_HIT events
    playerDiedHandle = EVENT_MGR.subscribe(GameEventType::PLAYER_HIT, [this](const GameEventData& event) {
        // Only respond if this is the player that was hit
        if (event.playerHit.player == this)
        {
            onPlayerHit();
        }
    });
}

Player::~Player()
{
    // Unsubscribe from events (EventManager handles this automatically on destruction)
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
    sprite->setFlipH(true);
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
    sprite->setFlipH(false);
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

void Player::update(float dt)
{
    if (shotCounter > 0) shotCounter--;

    if (dead)
    {
        // Update death action (rotation + physics trajectory)
        if (deathAction)
        {
            bool running = deathAction->update(dt);
            if (!running)
            {
                // Death action complete (fallen below screen)
                deathAction.reset();

                if (lives > 0)
                {
                    lives--;
                    revive();
                    return;
                }
                else
                {
                    playing = false;
                }
            }
        }
    }
    else
    {
        // Handle immunity blinking effect
        // TODO: handle this with a BlinkAction
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
 * Event handler for PLAYER_HIT events.
 * Creates and starts the death action when player is hit by a ball.
 */
void Player::onPlayerHit()
{
    if (dead || deathAction) return;  // Already dead or death action running

    // Create death action with trajectory
    // Thrown to the right with upward velocity
    int xVel = (facing == FacingDirection::RIGHT) ? 5 : -5;
    deathAction = std::make_unique<PlayerDeadAction>(this, xVel, -12.0f);
    deathAction->start();
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