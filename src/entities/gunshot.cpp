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
GunShot::GunShot(Scene* scn, Player* pl, SpriteSheet* sheet)
    : Shot(scn, pl, WeaponType::GUN)
    , spriteSheet(sheet)
{
    // Clone animation template from AppData (no need to redefine states for each bullet)
    auto clonedAnim = AppData::instance().stageRes.gunBulletAnim->clone();
    animController = std::unique_ptr<StateMachineAnim>(
        dynamic_cast<StateMachineAnim*>(clonedAnim.release())
    );

    // Set callback to kill shot when impact animation completes
    animController->setOnStateComplete([this](const std::string& state) {
        if (state == "impact")
        {
            kill();
        }
    });

    animController->setState("flight_intro");

    // Player uses bottom-middle coordinates (X = center, Y = bottom)
    // Calculate shot spawn position based on player facing direction
    float halfWidth = pl->getWidth() / 2.0f;
    float spriteOffset = pl->getSprite()->getXOff();

    // When facing left, spawn on left side; when facing right, spawn on right side
    if (pl->getFacing() == FacingDirection::LEFT)
    {
        // Convert center to left edge, then apply offsets
        xPos = xInit = pl->getX() - halfWidth + spriteOffset + 5;
    }
    else  // FacingDirection::RIGHT
    {
        // Player X is already center; add offset for right-side spawn
        xPos = xInit = pl->getX() + spriteOffset + 14;
    }

    // Player Y is bottom; convert to top and add offset for weapon position
    yPos = yInit = pl->getY() - pl->getHeight() + 2.0f;
}

GunShot::~GunShot()
{
}

/**
 * Update gun bullet movement and animation
 *
 * Advances animation frames and moves the bullet upward during flight states.
 * During impact state, bullet stays in place while playing impact animation.
 *
 * @param dt Delta time in seconds
 */
void GunShot::update(float dt)
{
    // Convert dt from seconds to milliseconds for animation controller
    float dtMs = dt * 1000.0f;
    animController->update(dtMs);

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
 * Uses AABB collision detection with the gun bullet's sprite bounds.
 * The collision box properly represents the bullet sprite, so we can use
 * the generic intersection test instead of custom point-based logic.
 */
bool GunShot::collision(Floor* fl)
{
    return intersects(getCollisionBox(), fl->getCollisionBox());
}

/**
 * Get the current sprite frame
 * Used for debug bounding box visualization
 */
Sprite* GunShot::getCurrentSprite() const
{
    return spriteSheet->getFrame(animController->getCurrentFrame());
}

/**
 * Get collision box for the gun bullet
 *
 * Unlike harpoon which has a chain, gun bullets are small animated sprites.
 * The collision box matches the current sprite frame dimensions.
 */
CollisionBox GunShot::getCollisionBox() const
{
    Sprite* sprite = getCurrentSprite();
    if (!sprite)
    {
        // Fallback if sprite not found (shouldn't happen)
        return { (int)xPos, (int)yPos, 16, 16 };
    }

    return { (int)xPos, (int)yPos, sprite->getWidth(), sprite->getHeight() };
}
