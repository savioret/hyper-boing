#include "main.h"
#include "gunshot.h"
#include "../core/animspritesheet.h"
#include "../game/scene.h"
#include "../game/platform.h"
#include "player.h"
#include "../core/graph.h"
#include "../core/spritesheet.h"
#include "../core/animcontroller.h"
#include "../core/coordhelper.h"

/**
 * GunShot constructor
 *
 * Initializes an animated bullet projectile using AnimSpriteSheet.
 *
 * @param scn The scene this shot belongs to
 * @param pl The player who fired the shot
 * @param animSheet AnimSpriteSheet containing gun bullet frames and animation
 */
GunShot::GunShot(Scene* scn, Player* pl, AnimSpriteSheet* animSheet)
    : Shot(scn, pl, WeaponType::GUN)
{
    // Clone the AnimSpriteSheet template for this bullet instance
    anim = animSheet->clone();

    // Set callback to kill shot when die animation completes
    anim->setOnStateComplete([this](const std::string& state) {
        if (state == "die")
        {
            kill();
        }
    });

    //sprite->setState("flight_intro");

    // Player uses bottom-middle coordinates (X = center, Y = bottom)
    // Calculate shot spawn position based on player facing direction
    float halfWidth = pl->getWidth() / 2.0f;

    // When facing left, spawn on left side; when facing right, spawn on right side
    if (pl->getFacing() == FacingDirection::LEFT)
    {
        xPos = xInit = pl->getX() - 10;
    }
    else  // FacingDirection::RIGHT
    {
        // Player X is already center; add offset for right-side spawn
        xPos = xInit = pl->getX() + 10;
    }

    // Player Y is bottom; convert to top and add offset for weapon position
    yPos = yInit = pl->getY() - pl->getHeight() + 5.0f;

    // Spawn muzzle flash at the weapon X, player head Y
    AnimSpriteSheet* sparkTmpl = gameinf.getStageRes().gunSparkAnim.get();
    //int sparkW, sparkH;
    //sparkTmpl->getBoundingBoxSize(sparkW, sparkH);
    if ( sparkTmpl )
        scene->spawnEffect(sparkTmpl, (int)xPos, yPos);// (int)( pl->getY() - pl->getHeight() + 2 ));
}

GunShot::~GunShot()
{
}

/**
 * Update gun bullet movement and animation
 *
 * Advances animation frames and moves the bullet upward during flight states.
 * During die state, bullet stays in place while playing die animation.
 *
 * @param dt Delta time in seconds
 */
void GunShot::update(float dt)
{
    // Convert dt from seconds to milliseconds for animation controller
    float dtMs = dt * 1000.0f;
    anim->update(dtMs);

    if (!isDead())
    {
        if (!inImpact)
        {
            // Normal upward movement
            if (yPos <= Stage::MIN_Y)
            {
                onCeilingHit();  // Trigger die animation
            }
            else
            {
                yPos -= weaponSpeed;
            }
        }
        // During die state, bullet stays in place while animation plays
        // The onStateComplete callback will call kill() when done
    }
}

/**
 * Trigger die animation sequence
 *
 * Switches to die state and starts playing frames 5→6
 */
void GunShot::triggerImpact(float impactY)
{
    if (!inImpact)
    {
        inImpact = true;
        anim->setState("die");
        player->looseShoot();  // Decrement player's shot count
        yPos = impactY + anim->getHeight();  // Align Y position to impacted element (sprite pixels are top aligned)
    }
}

/**
 * Draw the animated bullet sprite
 *
 * Renders the current frame from the sprite sheet.
 */
void GunShot::draw(Graph* graph)
{
    Sprite* frame = anim->getCurrentSprite();

    if (frame)
    {
        int renderX = toRenderX(xPos, anim->getWidth());
        int renderY = toRenderY(yPos, anim->getHeight());

        graph->draw(frame, renderX, renderY);
    }
}

/**
 * Handle floor collision - trigger die animation
 */
void GunShot::onFloorHit(Platform* f)
{
    triggerImpact(f->getY() + f->getHeight());
}

/**
 * Handle ceiling collision - trigger die animation
 */
void GunShot::onCeilingHit()
{
    triggerImpact(Stage::MIN_Y);
}

/**
 * Handle ball collision - instant kill (no die animation)
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
bool GunShot::collision(Platform* fl)
{
    return intersects(getCollisionBox(), fl->getCollisionBox());
}

/**
 * Get collision box for the gun bullet
 *
 * Unlike harpoon which has a chain, gun bullets are small animated sprites.
 * The collision box matches the current sprite frame dimensions.
 */
CollisionBox GunShot::getCollisionBox() const
{
    Sprite* spr = getCurrentSprite();

    // Visual position matches draw() logic:
    int visualX = toRenderX(getX(), anim->getWidth());
    int visualY = toRenderY(getY(), anim->getHeight());

    // Apply collision margins (same as original Ball::collision logic)
    return {
        visualX,            // Left margin
        visualY,            // Top offset
        spr->getWidth(),    // Width minus both side margins
        spr->getHeight()    // Height minus top offset
    };
}
