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
    , animController(std::make_unique<StateMachineAnim>())
{
    // Set up animation states
    // flight_intro: 0→1→2→3→4, then auto-transition to flight_loop
    animController->addState("flight_intro", {0, 1, 2, 3, 4}, 9, false, "flight_loop");
    // flight_loop: oscillate 3↔4 forever
    animController->addState("flight_loop", {3, 4}, 9, true);
    // impact: 5→6, then stays on 6 (we check for completion to kill)
    animController->addState("impact", {5, 6}, 9, false);

    // Set callback to kill shot when impact animation completes
    animController->setOnStateComplete([this](const std::string& state) {
        if (state == "impact")
        {
            kill();
        }
    });

    animController->setState("flight_intro");

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
 * Advances animation frames and moves the bullet upward during flight states.
 * During impact state, bullet stays in place while playing impact animation.
 */
void GunShot::move()
{
    // Update animation controller
    animController->update();

    if (!isDead())
    {
        if (!inImpact)
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
        // During impact state, bullet stays in place while animation plays
        // The onStateComplete callback will call kill() when done
    }
}

/**
 * Trigger impact animation sequence
 *
 * Switches to impact state and starts playing frames 5→6
 */
void GunShot::triggerImpact()
{
    if (!inImpact)
    {
        inImpact = true;
        animController->setState("impact");
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
    Sprite* frame = spriteSheet->getFrame(animController->getCurrentFrame());

    if (frame)
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
    return spriteSheet->getFrame(animController->getCurrentFrame());
}
