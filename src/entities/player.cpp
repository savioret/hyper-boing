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
    
    // Initialize sprites from global resources
    clearSprites();
    // Assuming max 16 frames based on usage (ANIM_DEAD=8)
    for(int i = 0; i < 16; i++) {
        addSprite(&gameinf.getBmp().player[id][i]);
    }
    
    setFrame(ANIM_SHOOT); // Match legacy: sprite = &...[ANIM_SHOOT]
    
    float startX = 200.0f + 100.0f * id;
    float startY = (float)(MAX_Y - getHeight());
    
    setPos(startX, startY);

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
    setFrame(ANIM_SHOOT);

    float startX = 200.0f + 100.0f * id;
    float startY = (float)(MAX_Y - getHeight());
    setPos(startX, startY);

    xDir = 5;
    yDir = -4;
}

void Player::moveLeft()
{
    facing = FacingDirection::LEFT;  // Update facing direction
    setFlipH(true);
    if (x > MIN_X - 10)
        x -= moveIncrement;

    if (frame >= ANIM_SHOOT - 1)
    {
        setFrame(ANIM_WALK);
        shotCounter = animSpeed;
    }
    else if (!shotCounter)
    {
        if (frame < ANIM_WALK + 4) setFrame(frame + 1);
        else setFrame(ANIM_WALK);
        shotCounter = animSpeed;
    }
    else shotCounter--;
}

void Player::moveRight()
{
    facing = FacingDirection::RIGHT;  // Update facing direction
    setFlipH(false);
    if (x + getWidth() < MAX_X - 5)
        x += moveIncrement;

    if (frame >= ANIM_SHOOT - 1)
    {
        setFrame(ANIM_WALK);
        shotCounter = animSpeed;
    }
    else if (!shotCounter)
    {
        if (frame < ANIM_WALK + 4) setFrame(frame + 1);
        else setFrame(ANIM_WALK);
        shotCounter = animSpeed;
    }

    else shotCounter--;
}

// void Player::setFrame(int f) // Inherited

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
        setFrame(ANIM_SHOOT + 1);
    }
}

void Player::stop()
{
    if (frame != ANIM_SHOOT)
    {
        if (frame == ANIM_SHOOT + 1)
            if (!shotCounter)
            {
                setFrame(ANIM_SHOOT);				
                shotCounter = shotInterval;
            }
            else shotCounter--;
        else
        {
            setFrame(ANIM_SHOOT);
        }
    }
    if (x + getWidth() > MAX_X - 10) x = (float)(MAX_X - 16 - getWidth());
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