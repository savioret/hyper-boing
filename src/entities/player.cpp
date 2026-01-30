#include "main.h"
#include "eventmanager.h"
#include "playerdeadaction.h"
#include "asepriteloader.h"
#include "logger.h"
#include "../core/coordhelper.h"
#include <cstring>
#include <SDL.h>

Player::Player(int id)
    : id(id), deathAction(nullptr), currentState(PlayerState::IDLE)
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

    // Subscribe to LEVEL_CLEAR events for victory animation
    levelClearHandle = EVENT_MGR.subscribe(GameEventType::LEVEL_CLEAR, [this](const GameEventData& event) {
        onLevelClear();
    });

    // Subscribe to STAGE_LOADED events to reset animation state
    stageLoadedHandle = EVENT_MGR.subscribe(GameEventType::STAGE_LOADED, [this](const GameEventData& event) {
        onStageLoaded();
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
    animController->addState("shoot", {ANIM_SHOOT + 1}, 1, true);
    animController->addState("win", {ANIM_WIN}, 1, true);
    animController->addState("dead", {ANIM_DEAD}, 1, true);
    animController->setState("idle");

    // Load walk animation from Aseprite JSON
    const char* playerPrefix = (id == 0) ? "p1" : "p2";
    walkSheet = std::make_unique<SpriteSheet>();
    char walkPath[256];
    std::snprintf(walkPath, sizeof(walkPath), "assets/graph/players/%swalk.json", playerPrefix);
    
    walkAnim = AsepriteLoader::load(&appGraph, walkPath, *walkSheet);
    if (!walkAnim)
    {
        LOG_WARNING("Failed to load walk animation for player %d", id + 1);
    }

    // Load victory animation from Aseprite JSON
    victorySheet = std::make_unique<SpriteSheet>();
    char victoryPath[256];
    std::snprintf(victoryPath, sizeof(victoryPath), "assets/graph/players/%svictory.json", playerPrefix);

    victoryAnim = AsepriteLoader::load(&appGraph, victoryPath, *victorySheet);
    if (!victoryAnim)
    {
        LOG_WARNING("Failed to load victory animation for player %d", id + 1);
    }

    // Set initial state
    currentState = PlayerState::IDLE;
    setFrame(ANIM_SHOOT); // Initial frame

    // Position uses bottom-middle coordinates:
    // X = horizontal center of sprite, Y = bottom of sprite (ground level)
    float startX = 200.0f + 100.0f * id + getWidth() / 2.0f;
    float startY = (float)MAX_Y;

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
    setState(PlayerState::IDLE);

    // Position uses bottom-middle coordinates
    float startX = 200.0f + 100.0f * id + getWidth() / 2.0f;
    float startY = (float)MAX_Y;
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
    // X is center; check if left edge (x - width/2) stays within bounds
    if (x - getWidth() / 2.0f > MIN_X - 10)
        x -= moveIncrement;

    // Transition to walk animation if not already walking
    if (currentState != PlayerState::WALKING &&
        currentState != PlayerState::VICTORY &&
        currentState != PlayerState::DEAD)
    {
        setState(PlayerState::WALKING);
    }
}

void Player::moveRight()
{
    facing = FacingDirection::RIGHT;  // Update facing direction
    setFlipH(false);
    // X is center; check if right edge (x + width/2) stays within bounds
    if (x + getWidth() / 2.0f < MAX_X - 5)
        x += moveIncrement;

    // Transition to walk animation if not already walking
    if (currentState != PlayerState::WALKING &&
        currentState != PlayerState::VICTORY &&
        currentState != PlayerState::DEAD)
    {
        setState(PlayerState::WALKING);
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
    setState(PlayerState::SHOOTING);
}

void Player::stop()
{
    // Exit walk mode
    if (currentState == PlayerState::WALKING)
    {
        setState(PlayerState::IDLE);
    }

    // Transition shoot to idle when cooldown expires
    if (currentState == PlayerState::SHOOTING && shotCounter == 0)
    {
        setState(PlayerState::IDLE);
    }

    // Emergency clamp: ensure right edge doesn't exceed boundary
    // X is center, so right edge = x + width/2
    if (x + getWidth() / 2.0f > MAX_X - 10)
        x = (float)(MAX_X - 16) - getWidth() / 2.0f;
}

void Player::update(float dt)
{
    if (shotCounter > 0) shotCounter--;

    // Convert dt from seconds to milliseconds for animation controllers
    float dtMs = dt * 1000.0f;

    // Update appropriate animation controller based on state
    switch (currentState)
    {
        case PlayerState::VICTORY:
            if (victoryAnim) {
                victoryAnim->update(dtMs);
                LOG_TRACE("Player %d victory anim update - frame: %d", id + 1, victoryAnim->getCurrentFrame());
            }
            break;

        case PlayerState::WALKING:
            if (walkAnim) {
                walkAnim->update(dtMs);
            }
            break;

        case PlayerState::IDLE:
        case PlayerState::SHOOTING:
        case PlayerState::DEAD:
            animController->update(dtMs);
            setFrame(animController->getCurrentFrame());
            break;
    }

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
    setState(PlayerState::DEAD);

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

/**
 * Event handler for LEVEL_CLEAR events.
 * Triggers the victory animation loaded from Aseprite JSON.
 */
void Player::onLevelClear()
{
    if (victoryAnim && !isDead())
    {
        setState(PlayerState::VICTORY);
    }
    else
    {
        LOG_WARNING("Player %d cannot enter victory mode (anim: %s, dead: %s)",
                   id + 1, victoryAnim ? "yes" : "no", isDead() ? "yes" : "no");
    }
}

/**
 * Event handler for STAGE_LOADED events.
 * Resets animation state when starting a new stage.
 */
void Player::onStageLoaded()
{
    // Reset animation state to idle
    setState(PlayerState::IDLE);
}

/**
 * Draw player sprite.
 * Player position is stored in bottom-middle coordinates; this method
 * converts to top-left for SDL rendering using coordhelper.
 */
void Player::draw(Graph* graph)
{
    if (!visible) return;

    Sprite* spr = getActiveSprite();
    if (!spr) return;

    RenderProps props(toRenderX(getX(), spr), toRenderY(getY(), spr));
    props.flipH = getFlipH();
    props.rotation = getRotation();
    props.scale = getScale();
    props.alpha = getAlpha() / 255.0f;

    graph->drawEx(spr, props);
}

Sprite* Player::getActiveSprite() const
{
    switch (currentState)
    {
        case PlayerState::WALKING:
            if (walkAnim && walkSheet)
                return walkSheet->getFrame(walkAnim->getCurrentFrame());
            break;

        case PlayerState::VICTORY:
            if (victoryAnim && victorySheet)
                return victorySheet->getFrame(victoryAnim->getCurrentFrame());
            break;

        case PlayerState::IDLE:
        case PlayerState::SHOOTING:
        case PlayerState::DEAD:
        default:
            return getCurrentSprite();  // Standard sprite from sprites array
    }

    // Fallback if animation not loaded
    return getCurrentSprite();
}

CollisionBox Player::getCollisionBox() const
{
    Sprite* spr = getActiveSprite();
    if (!spr) {
        // Fallback for no sprite
        int renderX = toRenderX(getX(), getWidth());
        int renderY = toRenderY(getY(), getHeight());
        return { renderX + 5, renderY + 3, getWidth() - 10, getHeight() - 3 };
    }

    // Visual position matches draw() logic:
    // toRenderX/Y gives top-left of source canvas, drawEx adds xOff/yOff
    int visualX = toRenderX(getX(), spr) + spr->getXOff();
    int visualY = toRenderY(getY(), spr) + spr->getYOff();

    // Apply collision margins (same as original Ball::collision logic)
    return {
        visualX + 5,            // Left margin
        visualY + 3,            // Top offset
        spr->getWidth() - 10,   // Width minus both side margins
        spr->getHeight() - 3    // Height minus top offset
    };
}

void Player::setState(PlayerState newState)
{
    if (currentState == newState) return;

    currentState = newState;

    switch (newState)
    {
        case PlayerState::IDLE:
            animController->setState("idle");
            setFrame(ANIM_SHOOT);
            break;

        case PlayerState::WALKING:
            if (walkAnim) {
                walkAnim->reset();
            }
            break;

        case PlayerState::SHOOTING:
            animController->setState("shoot");
            break;

        case PlayerState::VICTORY:
            if (victoryAnim) {
                victoryAnim->reset();
                LOG_INFO("Player %d entering victory mode", id + 1);
            }
            break;

        case PlayerState::DEAD:
            animController->setState("dead");
            break;
    }
}