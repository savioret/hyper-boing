#include "main.h"
#include "eventmanager.h"
#include "playerdeadaction.h"
#include "animspritesheet.h"
#include "logger.h"
#include "ladder.h"
#include "../game/scene.h"
#include "../core/coordhelper.h"
#include "../core/sprite.h"
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
    currentWeapon = WeaponType::HARPOON;  // Default weapon
    maxShoots = WeaponConfig::get(currentWeapon).maxShots;
    numShoots = 0;
    lives = 3;
    visible = true;
    immuneCounter = 0;
    playing = true;
    hasShield = false;

    shotInterval = 15;
    animSpeed = shotCounter = 10;
    score = 0;
    moveIncrement = 3;
    facing = FacingDirection::RIGHT;  // Default facing right

    // Reset render properties
    renderProps = RenderProps();

    // Climbing initialization
    currentLadder = nullptr;
    climbSpeed = 2.0f;
    climbingMoving = false;

    // Physics initialization
    yVelocity = 0.0f;
    grounded = true;  // Start grounded at Stage::MAX_Y

    // Initialize fallback sprites from global resources
    // Load 9 player sprite frames (0-8: walk, shoot, win, dead)
    std::vector<Sprite*> fallbackSprites;
    for(int i = 0; i <= ANIM_DEAD; i++) {
        fallbackSprites.push_back(&gameinf.getBmp().player[id][i]);
    }
    sprite.setFallbackSprites(fallbackSprites);
    sprite.setFallbackFrame(ANIM_SHOOT);  // Start with idle frame (uses ANIM_SHOOT)

    // Load walk animation from Aseprite JSON
    const char* playerPrefix = (id == 0) ? "p1" : "p2";
    char walkPath[256];
    std::snprintf(walkPath, sizeof(walkPath), "assets/graph/players/%swalk.json", playerPrefix);

    walkAnim = AnimSpriteSheet::load(&appGraph, walkPath);
    if (walkAnim)
    {
        sprite.registerAnimation("walk", walkAnim.get());
    }
    else
    {
        LOG_WARNING("Failed to load walk animation for player %d", id + 1);
    }

    // Load victory animation from Aseprite JSON
    char victoryPath[256];
    std::snprintf(victoryPath, sizeof(victoryPath), "assets/graph/players/%svictory.json", playerPrefix);

    victoryAnim = AnimSpriteSheet::load(&appGraph, victoryPath);
    if (victoryAnim)
    {
        sprite.registerAnimation("victory", victoryAnim.get());
    }
    else
    {
        LOG_WARNING("Failed to load victory animation for player %d", id + 1);
    }

    // Load climbing animation from Aseprite JSON with frameTags
    char climbPath[256];
    std::snprintf(climbPath, sizeof(climbPath), "assets/graph/players/%sclimbing.json", playerPrefix);
    climbAnim = AnimSpriteSheet::loadAsStateMachine(&appGraph, climbPath);
    if (climbAnim)
    {
        sprite.registerAnimation("climb", climbAnim.get());

        // Set callback for when standup completes → transition to IDLE
        climbAnim->setOnStateComplete([this](const std::string& stateName) {
            if (stateName == "standup" && currentState == PlayerState::WAKING_UP) {
                setState(PlayerState::IDLE);
            }
        });
    }

    // Clone shield effect animation from shared template
    if (gameinf.getStageRes().shieldAnim)
    {
        shieldAnimInstance = gameinf.getStageRes().shieldAnim->clone();
    }

    // Set initial state - use fallback sprite (idle)
    currentState = PlayerState::IDLE;
    sprite.useAsFallback();

    // Position uses bottom-middle coordinates:
    // X = horizontal center of sprite, Y = bottom of sprite (ground level)
    float startX = 200.0f + 100.0f * id;
    float startY = (float)Stage::MAX_Y;

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

    currentWeapon = WeaponType::HARPOON;  // Reset to default on death
    maxShoots = WeaponConfig::get(currentWeapon).maxShots;
    numShoots = 0;
    playing = true;

    shotInterval = 15;
    animSpeed = shotCounter = 10;
    facing = FacingDirection::RIGHT;  // Reset facing on revive

    // Reset render properties
    renderProps = RenderProps();

    // Reset physics state
    yVelocity = 0.0f;
    grounded = true;
    currentLadder = nullptr;
    climbingMoving = false;

    // Reset animation state to idle
    setState(PlayerState::IDLE);

    // Position uses bottom-middle coordinates
    float startX = 200.0f + 100.0f * id;
    float startY = (float)Stage::MAX_Y;
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
    if (getX() - getWidth() / 2.0f > Stage::MIN_X)
    {
        float oldX = getX();
        setX(getX() - moveIncrement);
        LOG_TRACE("Player %d moveLeft: x=%.1f -> %.1f, y=%.1f, grounded=%d",
                  id + 1, oldX, getX(), getY(), grounded);
    }

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
    if (getX() + getWidth() / 2.0f < Stage::MAX_X)
    {
        float oldX = getX();
        setX(getX() + moveIncrement);
        LOG_TRACE("Player %d moveRight: x=%.1f -> %.1f, y=%.1f, grounded=%d",
                  id + 1, oldX, getX(), getY(), grounded);
    }

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

    // Don't change state if climbing - stay in climbing mode
    // (shooting animation can be shown separately if needed)
    if (currentState != PlayerState::CLIMBING)
    {
        setState(PlayerState::SHOOTING);
    }
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
    //if (getX() + getWidth() / 2.0f > Stage::MAX_X - 16)
    //    setX((float)(Stage::MAX_X - 22) - getWidth() / 2.0f);
}

// Climbing methods

void Player::startClimbing(Ladder* ladder)
{
    if (!ladder || isDead()) return;

    currentLadder = ladder;
    setState(PlayerState::CLIMBING);

    // Align player's grip point (hands) with ladder center
    // The climbing sprite's grip point is offset from the canvas center,
    // so we need to adjust the player's X position accordingly
    setX((float)ladder->getCenterX());
}

void Player::stopClimbing()
{
    setState(PlayerState::IDLE);
    //if(currentLadder)
    //    x = (float)currentLadder->getCenterX() + currentLadder->getTileWidth() / 2;
    currentLadder = nullptr;
}

void Player::climbUp()
{
    if (!currentLadder || isDead()) return;

    climbingMoving = true;  // Mark as moving on ladder

    float newY = yPos - climbSpeed;
    float ladderTop = (float)currentLadder->getTopY();

    if (newY <= ladderTop)
    {
        // Reached top - transition to WAKING_UP state
        LOG_TRACE("Player %d REACHED LADDER TOP: y=%.1f -> ladderTop=%.1f, ladderCenterX=%d",
                  id + 1, yPos, ladderTop, currentLadder->getCenterX());
        yPos = ladderTop;
        setState(PlayerState::WAKING_UP);

        // Set animation to "standup" state (frame 4, 300ms, one-shot)
        if (climbAnim) {
            climbAnim->setState("standup");
        }
    }
    else
    {
        yPos = newY;
    }

    // Animation is updated in Player::update() using proper dt
}

void Player::climbDown()
{
    if (!currentLadder || isDead()) return;

    climbingMoving = true;  // Mark as moving on ladder

    float newY = yPos + climbSpeed;
    float ladderBottom = (float)currentLadder->getBottomY();

    if (newY >= ladderBottom)
    {
        // Reached bottom - exit ladder at ground
        yPos = ladderBottom;
        stopClimbing();
    }
    else
    {
        yPos = newY;
    }

    // Animation is updated in Player::update() using proper dt
}

bool Player::canEnterLadder(Ladder* ladder) const
{
    if (!ladder) return false;

    // Use 50% horizontal overlap rule from collisionbox.h
    return has50PercentOverlap(getCollisionBox(), ladder->getCollisionBox());
}

void Player::update(float dt, Scene* scene)
{
    if (shotCounter > 0) shotCounter--;

    // Convert dt from seconds to milliseconds for animation controllers
    float dtMs = dt * 1000.0f;

    // Update appropriate animation controller based on state
    switch (currentState)
    {
        case PlayerState::VICTORY:
        case PlayerState::WALKING:
            // These use sprite's active animation
            sprite.update(dtMs);
            break;

        case PlayerState::CLIMBING:
            if (climbAnim) {
                // Determine which climbing animation state to use based on player actions
                std::string desiredState;
                if (shotCounter > 0) {
                    // Currently shooting - show shoot animation
                    desiredState = "shoot";
                } else if (climbingMoving) {
                    // Moving up or down - show climb animation
                    desiredState = "climb";
                } else {
                    // Idle on ladder - show stop animation
                    desiredState = "stop";
                }

                // Only change state if different from current state
                if (climbAnim->getStateName() != desiredState) {
                    climbAnim->setState(desiredState);
                }

                climbAnim->update(dtMs);
            }
            // Reset climbing movement flag for next frame
            climbingMoving = false;
            break;

        case PlayerState::WAKING_UP:
            if (climbAnim) {
                climbAnim->update(dtMs);
            }
            break;

        case PlayerState::IDLE:
        case PlayerState::SHOOTING:
        case PlayerState::DEAD:
            // These use fallback sprites with specific frames
            // No animation update needed - just static frames
            break;
    }

    // Update shield animation regardless of player state
    if (hasShield && shieldAnimInstance)
    {
        shieldAnimInstance->update(dtMs);
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
        // Apply gravity/platform physics
        updatePhysics(dt, scene);

        // Handle immunity blinking effect
        if (immuneCounter)
        {
            immuneCounter--;
            visible = !visible;
            if (!immuneCounter) visible = true;
        }
    }
}

void Player::updatePhysics(float dt, Scene* scene)
{
    // Don't apply gravity while climbing or dead
    // Check both state AND currentLadder for safety (prevents falling if state desyncs)
    if (isClimbing() || currentLadder != nullptr || isDead()) return;

    // Find what's below us
    float groundY = scene->findGroundLevel(this);

    // Are we on the ground?
    float feetY = getY();  // Player Y is already feet position (bottom-middle)
    float distToGround = groundY - feetY;

    if (distToGround <= GROUND_SNAP_TOLERANCE)  // On ground or within tolerance
    {
        // On ground
        if (!grounded)
        {
            // Just landed
            LOG_TRACE("Player %d LANDED: feetY=%.1f, groundY=%.1f, distToGround=%.1f",
                      id + 1, feetY, groundY, distToGround);
            grounded = true;
            yVelocity = 0.0f;
        }
        yPos = groundY;  // Snap to ground
    }
    else
    {
        // In air - apply gravity
        if (grounded)
        {
            LOG_TRACE("Player %d START FALLING: feetY=%.1f, groundY=%.1f, distToGround=%.1f",
                      id + 1, feetY, groundY, distToGround);
        }
        grounded = false;
        yVelocity += GRAVITY;
        if (yVelocity > MAX_FALL_SPEED) yVelocity = MAX_FALL_SPEED;

        yPos += yVelocity;

        // Check if we've passed through ground
        if (yPos >= groundY)
        {
            LOG_TRACE("Player %d LANDED (passed through): y=%.1f -> groundY=%.1f",
                      id + 1, yPos, groundY);
            yPos = groundY;
            grounded = true;
            yVelocity = 0.0f;
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
    // Reset power-up states on death
    hasShield = false;
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
    float xVel = (facing == FacingDirection::RIGHT) ? 5 : -5;
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
    // Reset double-shoot buff when weapon changes
}

/**
 * Event handler for LEVEL_CLEAR events.
 * Triggers the victory animation loaded from Aseprite JSON.
 */
void Player::onLevelClear()
{
    if (isDead())
    {
        LOG_WARNING("Player %d cannot enter victory mode - player is dead", id + 1);
        return;
    }
    
    // Clear immunity state to prevent blinking during victory animation
    immuneCounter = 0;
    visible = true;
    
    if (victoryAnim)
    {
        setState(PlayerState::VICTORY);
        LOG_INFO("Player %d entering victory mode", id + 1);
    }
    else
    {
        LOG_ERROR("Player %d cannot enter victory mode - victoryAnim not loaded!", id + 1);
        // Fallback: at least show idle pose instead of walking
        setState(PlayerState::IDLE);
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

    Sprite* spr = sprite.getActiveSprite();
    if (!spr) return;

    RenderProps props = renderProps;
    props.x = toRenderX(getX(), sprite.getWidth());
    props.y = toRenderY(getY(), sprite.getHeight());

    // Draw shield effect animation behind player
    if (hasShield && shieldAnimInstance)
    {
        Sprite* shieldSpr = shieldAnimInstance->getCurrentSprite();
        if (shieldSpr)
        {
            RenderProps shieldProps;
            int shieldX = toRenderX(getX(), shieldAnimInstance->getWidth());
            int shieldY = toRenderY(getY(), shieldAnimInstance->getHeight());

            //shieldProps.x = static_cast< int >(getX()) - shieldAnimInstance->getWidth() / 2;
            //shieldProps.y = static_cast<int>(getY()) - spr->getHeight() / 2 - shieldAnimInstance->getHeight() / 2;
            graph->draw(shieldSpr, shieldX, shieldY+11);
        }
    }

    graph->drawEx(spr, props);
}

AnimSpriteSheet* Player::getActiveAnim() const
{
    switch (currentState)
    {
        case PlayerState::WALKING:
        case PlayerState::VICTORY:
            // These use sprite's active animation
            return sprite.getActiveAnimation();

        case PlayerState::CLIMBING:
        case PlayerState::WAKING_UP:
            // Climbing uses its own animation with internal states
            if (climbAnim)
                return climbAnim.get();
            break;

        case PlayerState::IDLE:
        case PlayerState::SHOOTING:
        case PlayerState::DEAD:
        default:
            // Use fallback sprites from sprite (these don't have animations)
            return nullptr;
    }

    // Fallback
    return nullptr;
}

CollisionBox Player::getCollisionBox() const
{
    Sprite* spr = sprite.getActiveSprite();
    if (!spr) {
        // Return empty collision box if no sprite available
        return {0, 0, 0, 0};
    }

    // Visual position matches draw() logic:
    // toRenderX/Y gives top-left of source canvas, drawEx adds xOff/yOff
    int visualX = toRenderX(getX(), spr->getWidth());// +spr->getXOff();
    int visualY = toRenderY(getY(), spr->getHeight());// +spr->getYOff();

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

    // Safety: Clear currentLadder when transitioning OUT of climbing states to non-climbing states
    // Must check BEFORE assignment since we compare old vs new state
    if ((currentState == PlayerState::CLIMBING || currentState == PlayerState::WAKING_UP) &&
        (newState != PlayerState::CLIMBING && newState != PlayerState::WAKING_UP) &&
        currentLadder != nullptr)
    {
        LOG_TRACE("Player %d: setState clearing currentLadder (was climbing, now %d)", id + 1, (int)newState);
        currentLadder = nullptr;
    }

    currentState = newState;

    switch (newState)
    {
        case PlayerState::IDLE:
            sprite.useAsFallback();
            sprite.setFallbackFrame(ANIM_SHOOT);  // Idle uses ANIM_SHOOT frame
            break;

        case PlayerState::WALKING:
            if (walkAnim) {
                walkAnim->reset();
                sprite.setActiveAnimation("walk");
            }
            break;

        case PlayerState::CLIMBING:
            if (climbAnim) {
                climbAnim->reset();
                climbAnim->setState("climb");
                sprite.setActiveAnimation("climb");
            }
            break;

        case PlayerState::WAKING_UP:
            if (climbAnim) {
                climbAnim->setState("standup");
                // Keep using climb animation for standup
            }
            break;

        case PlayerState::SHOOTING:
            sprite.useAsFallback();
            sprite.setFallbackFrame(ANIM_SHOOT_POSE);  // Shooting uses ANIM_SHOOT + 1 frame
            break;

        case PlayerState::VICTORY:
            if (victoryAnim) {
                victoryAnim->reset();
                sprite.setActiveAnimation("victory");
                LOG_INFO("Player %d entering victory mode", id + 1);
            }
            break;

        case PlayerState::DEAD:
            sprite.useAsFallback();
            sprite.setFallbackFrame(ANIM_DEAD);
            break;
    }
}