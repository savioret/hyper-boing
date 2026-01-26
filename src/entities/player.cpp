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
    resetDead();  // Reset IGameObject's dead flag
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
    // Load 9 player sprite frames (0-8: walk, shoot, win, dead)
    for(int i = 0; i <= ANIM_DEAD; i++) {
        addSprite(&gameinf.getBmp().player[id][i]);
    }

    // Initialize animation controller with state machine
    animController = std::make_unique<StateMachineAnim>();
    animController->addState("idle", {ANIM_SHOOT}, 1, true);
    animController->addState("walk", {ANIM_WALK, ANIM_WALK+1, ANIM_WALK+2, ANIM_WALK+3, ANIM_WALK+4}, animSpeed, true);
    animController->addState("shoot", {ANIM_SHOOT + 1}, 1, true);
    animController->addState("win", {ANIM_WIN}, 1, true);
    animController->addState("dead", {ANIM_DEAD}, 1, true);
    animController->setState("idle");

    setFrame(ANIM_SHOOT); // Initial frame
    
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
    resetDead();  // Reset IGameObject's dead flag
    immuneCounter = 350;

    frame = 0;
    currentWeapon = WeaponType::HARPOON;  // Reset to default on death
    maxShoots = 2;
    numShoots = 0;
    playing = true;

    shotInterval = 15;
    animSpeed = shotCounter = 10;
    facing = FacingDirection::RIGHT;  // Reset facing on revive

    // Reset animation state to idle
    animController->setState("idle");
    setFrame(ANIM_SHOOT);

    float startX = 200.0f + 100.0f * id;
    float startY = (float)(MAX_Y - getHeight());
    setPos(startX, startY);

    xDir = 5;
    yDir = -4;

    // Fire PLAYER_REVIVED event
    GameEventData event(GameEventType::PLAYER_REVIVED);
    event.playerRevived.player = this;
    event.playerRevived.remainingLives = lives;
    EVENT_MGR.trigger(event);
}

void Player::moveLeft()
{
    facing = FacingDirection::LEFT;  // Update facing direction
    setFlipH(true);
    if (x > MIN_X - 10)
        x -= moveIncrement;

    // Transition to walk animation if not already walking
    if (animController->getStateName() != "walk")
    {
        animController->setState("walk");
    }
}

void Player::moveRight()
{
    facing = FacingDirection::RIGHT;  // Update facing direction
    setFlipH(false);
    if (x + getWidth() < MAX_X - 5)
        x += moveIncrement;

    // Transition to walk animation if not already walking
    if (animController->getStateName() != "walk")
    {
        animController->setState("walk");
    }
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
    animController->setState("shoot");
}

void Player::stop()
{
    const std::string& currentState = animController->getStateName();

    // Transition walk to idle immediately
    if (currentState == "walk")
    {
        animController->setState("idle");
    }
    // Transition shoot to idle when cooldown expires
    else if (currentState == "shoot" && shotCounter == 0)
    {
        animController->setState("idle");
    }

    if (x + getWidth() > MAX_X - 10) x = (float)(MAX_X - 16 - getWidth());
}

void Player::update(float dt)
{
    if (shotCounter > 0) shotCounter--;

    // Update animation controller and sync frame
    animController->update();
    setFrame(animController->getCurrentFrame());

    if (isDead())
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
    GameEventData event(GameEventType::SCORE_CHANGED);
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
 * Overrides IGameObject::kill() to add Player-specific death behavior.
 */
void Player::kill()
{
    IGameObject::kill();  // Sets dead flag and calls onDeath()
    animSpeed = 4;
}

/**
 * Called when player is first killed (from IGameObject::kill()).
 * Can be used for death effects, sounds, etc.
 */
void Player::onDeath()
{
    // Currently no additional logic needed here
    // Death animation is handled via PlayerDeadAction in onPlayerHit()
}

/**
 * Event handler for PLAYER_HIT events.
 * Creates and starts the death action when player is hit by a ball.
 */
void Player::onPlayerHit()
{
    if (isDead() || deathAction) return;  // Already dead or death action running

    // Set death animation state
    animController->setState("dead");

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