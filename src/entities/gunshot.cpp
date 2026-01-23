#include "main.h"
#include "gunshot.h"
#include "../game/scene.h"
#include "../game/floor.h"
#include "player.h"
#include "../core/graph.h"
#include "../core/spritesheet.h"

/**
 * GunShot constructor
 *
 * Initializes an animated bullet projectile using a sprite sheet.
 *
 * @param scn The scene this shot belongs to
 * @param pl The player who fired the shot
 * @param sheet Sprite sheet containing all gun bullet frames
 * @param xOffset Horizontal offset for multi-projectile weapons
 */
GunShot::GunShot(Scene* scn, Player* pl, SpriteSheet* sheet, int xOffset)
    : Shot(scn, pl, WeaponType::GUN, xOffset)
    , spriteSheet(sheet)
    , currentFrame(0)
    , frameDelay(9)      // Change frames every 9 game ticks (~150ms at 60 FPS)
    , frameCounter(frameDelay)
    , animState(AnimState::FLIGHT)
{
    // Calculate shot spawn position based on player facing direction
    float centerOffset = pl->getWidth() / 2.0f;
    float spriteOffset = pl->getSprite()->getXOff();

    // When facing left, spawn on left side; when facing right, spawn on right side
    if (pl->getFacing() == FacingDirection::LEFT)
    {
        xPos = xInit = pl->getX() + spriteOffset + +5 + xOffset;
    }
    else  // FacingDirection::RIGHT
    {
        xPos = xInit = pl->getX() + centerOffset + spriteOffset + 14 + xOffset;
    }

    yPos = yInit =  pl->getY() + 2.0f;  // Spawn slightly above player's Y position
}

GunShot::~GunShot()
{
}

/**
 * Update gun bullet movement and animation
 *
 * Advances animation frames and moves the bullet upward during FLIGHT state.
 * During IMPACT state, bullet stays in place while playing impact animation.
 */
void GunShot::move()
{
    // Update frame animation
    if (--frameCounter <= 0)
    {
        frameCounter = frameDelay;
        advanceFrame();
    }

    if (!isDead())
    {
        if (animState == AnimState::FLIGHT)
        {
            // Normal upward movement
            if (yPos <= MIN_Y)
            {
                onCeilingHit();  // Trigger impact animation
            }
            else
            {
                yPos -= weaponSpeed;
            }
        }
        else if (animState == AnimState::IMPACT)
        {
            // Stay in place during impact animation
            // Animation will call kill() when done (frame 6)
        }
    }
}

/**
 * Advance to next animation frame based on current state
 *
 * FLIGHT state: Sequence 0→1→2→3→4, then loops 3↔4
 * IMPACT state: Sequence 5→6, then kills the shot
 */
void GunShot::advanceFrame()
{
    if (animState == AnimState::FLIGHT)
    {
        // Sequence: 0→1→2→3→4, then loop 3↔4
        if (currentFrame < 3)
        {
            currentFrame++;
        }
        else if (currentFrame == 3)
        {
            currentFrame = 4;
        }
        else  // currentFrame == 4
        {
            currentFrame = 3;  // Loop back to 3
        }
    }
    else if (animState == AnimState::IMPACT)
    {
        // Sequence: 5→6→kill
        if (currentFrame == 5)
        {
            currentFrame = 6;
        }
        else if (currentFrame == 6)
        {
            kill();  // Impact animation complete
        }
    }
}

/**
 * Trigger impact animation sequence
 *
 * Switches to IMPACT state and starts playing frames 5→6
 */
void GunShot::triggerImpact()
{
    if (animState != AnimState::IMPACT)
    {
        animState = AnimState::IMPACT;
        currentFrame = 5;
        frameCounter = frameDelay;
        player->looseShoot();  // Decrement player's shot count
    }
}

/**
 * Draw the animated bullet sprite
 *
 * Renders the current frame from the sprite sheet.
 */
void GunShot::draw(Graph* graph)
{
    Sprite* frame = spriteSheet->getFrame(currentFrame);

    if ( frame )
    {
        graph->draw(frame, (int)xPos, (int)yPos);
    }
}

/**
 * Handle floor collision - trigger impact animation
 */
void GunShot::onFloorHit(Floor* f)
{
    triggerImpact();
}

/**
 * Handle ceiling collision - trigger impact animation
 */
void GunShot::onCeilingHit()
{
    triggerImpact();
}

/**
 * Handle ball collision - instant kill (no impact animation)
 */
void GunShot::onBallHit(Ball* b)
{
    player->looseShoot();
    kill();
}

/**
 * Check collision with floor
 *
 * Overrides base Shot::collision() which uses xPos + 8.
 * Gun bullets use centered sprites with negative offsets, so the collision
 * point should be at xPos (the sprite's center), not xPos + 8.
 */
bool GunShot::collision(Floor* fl)
{
    // Use xPos directly as the collision point (sprite center)
    // No +8 offset needed since gun bullet sprites are centered differently than harpoon
    if (xPos > fl->getX() - 1 && xPos < fl->getX() + fl->getWidth() + 1 &&
        fl->getY() + fl->getHeight() > yPos)
    {
        return true;
    }

    return false;
}

/**
 * Get the current sprite frame
 * Used for debug bounding box visualization
 */
Sprite* GunShot::getCurrentSprite() const
{
    return spriteSheet->getFrame(currentFrame);
}
