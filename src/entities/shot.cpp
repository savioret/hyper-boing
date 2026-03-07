#include "main.h"
#include "shot.h"
#include "../game/scene.h"
#include "player.h"
#include "../game/platform.h"

/**
 * Shot base constructor
 *
 * Calculates the origin of the shot based on the player position.
 * Player uses bottom-middle coordinates (X = center, Y = bottom).
 *
 * @param scn The scene this shot belongs to
 * @param pl The player who fired the shot
 * @param type The weapon type being used
 */
Shot::Shot(Scene* scn, Player* pl, WeaponType type)
    : scene(scn), player(pl), weaponType(type), audioChannel(-1)
{
    const WeaponConfig& config = WeaponConfig::get(type);
    weaponSpeed = config.speed;

    if (true || pl->isClimbing())
    {
        // While climbing, spawn centered on player's X (bottom-middle pivot)
        xPos = xInit = pl->getX();
    }
    else
    {
        // Player X is center; calculate spawn position based on facing direction
        float halfWidth = pl->getWidth() / 2.0f;

        // When facing left, spawn on left side; when facing right, spawn on right side
        if (pl->getFacing() == FacingDirection::LEFT)
        {
            // Convert center to left edge, then apply offsets
            xPos = xInit = pl->getX() - halfWidth - 2.0f;
        }
        else  // FacingDirection::RIGHT
        {
            // Player X is already center; add offset for right-side spawn
            xPos = xInit = pl->getX() + 5.0f;
        }
    }

    // Player Y is feet (bottom-middle pivot = Y is the bottom of the sprite).
    // yInit anchors the chain bottom at feet level.
    yPos = yInit = pl->getY();
}

/**
 * Default ball hit behavior - decrement player shot count and kill
 */
void Shot::onBallHit(Ball* b)
{
    player->looseShoot();
    kill();
}

/**
 * Default floor hit behavior - decrement player shot count and kill
 */
void Shot::onFloorHit(Platform* f)
{
    player->looseShoot();
    kill();
}

/**
 * Default ceiling hit behavior - decrement player shot count and kill
 */
void Shot::onCeilingHit()
{
    player->looseShoot();
    kill();
}

/**
 * Check collision with floor
 * @param fl Floor to check collision with
 * @return true if colliding, false otherwise
 *
 * Default implementation using AABB collision detection.
 * Can be overridden by subclasses for specialized behavior.
 */
bool Shot::collision(Platform* fl)
{
    return intersects(getCollisionBox(), fl->getCollisionBox());
}

/**
 * Called when shot dies - stops associated audio channel
 */
void Shot::onDeath()
{
    if (audioChannel >= 0)
    {
        AudioManager::instance().stopChannel(audioChannel);
        audioChannel = -1;
    }
}
