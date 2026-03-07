#include "main.h"
#include "clawshot.h"
#include "../game/scene.h"
#include "../game/platform.h"
#include "../game/stage.h"
#include "player.h"
#include "../core/appdata.h"
#include "../core/graph.h"
#include "../core/sprite.h"
#include "logger.h"

/**
 * ClawShot constructor
 *
 * Clones the claw sprite sheet template from shared resources and sets the
 * initial horizontal position so the sprite canvas is centered on the player,
 * matching HarpoonShot's positioning logic.
 */
ClawShot::ClawShot(Scene* scn, Player* pl, WeaponType type)
    : Shot(scn, pl, type), stuck(false), stuckTimer(0.0f)
{
    StageResources& res = gameinf.getStageRes();

    if (res.clawWeaponAnim)
        clawAnim = res.clawWeaponAnim->clone();

    if (res.clawWeaponYellowAnim)
        clawYellowAnim = res.clawWeaponYellowAnim->clone();

    // Position xPos so the canvas is centered on the player, with a small
    // horizontal offset toward the facing direction (mirrors HarpoonShot logic).
    int off = 10;
    float halfW = clawAnim ? clawAnim->getWidth() / 2.0f : 0.0f;
    if (pl->isClimbing())
    {
        xPos = xInit = pl->getX() - halfW;
    }
    else if (pl->getFacing() == FacingDirection::LEFT)
    {
        xPos = xInit = pl->getX() - halfW - off;
    }
    else  // FacingDirection::RIGHT
    {
        xPos = xInit = pl->getX() - halfW + off;
    }

    // Start the tip at player-head height: move up by the canvas height so the
    // bottom of the top sprite aligns with the player's feet (yInit).
    if (clawAnim)
    {
        Sprite* tipSpr = clawAnim->getFrame(2);
        if (tipSpr)
            yPos -= tipSpr->getHeight();
    }
}

/**
 * Update movement and stuck timer.
 */
void ClawShot::update(float dt)
{
    if (isDead()) return;

    if (stuck)
    {
        stuckTimer -= dt;
        if (stuckTimer <= 0.0f)
        {
            player->looseShoot();
            kill();
        }
        return;
    }

    // Flying upward
    if (yPos <= Stage::MIN_Y)
    {
        onCeilingHit();
    }
    else
    {
        yPos -= weaponSpeed;
    }
}

/**
 * Draw tip (or claw head when stuck) followed by tiled chain down to yInit.
 */
void ClawShot::draw(Graph* graph)
{
    if (!clawAnim) return;

    // Use yellow skin during the last second while stuck; default otherwise
    bool useYellow = stuck && stuckTimer <= WARN_THRESHOLD && clawYellowAnim;
    AnimSpriteSheet* skin = useYellow ? clawYellowAnim.get() : clawAnim.get();

    // Yellow skin: frame 0 = head, frame 1 = chain (no tip frame)
    // Default skin: frame 0 = head, frame 1 = chain, frame 2 = tip
    Sprite* headSpr  = skin->getFrame(0);
    Sprite* chainSpr = skin->getFrame(1);
    Sprite* tipSpr   = stuck ? nullptr : clawAnim->getFrame(2);

    if (!headSpr || !chainSpr) return;
    if (!stuck && !tipSpr) return;

    Sprite* topSpr = stuck ? headSpr : tipSpr;

    // Draw top sprite (xOff applied automatically by graph->draw)
    graph->draw(topSpr, (int)xPos, (int)yPos);

    // Tile the chain from below the top sprite down to yInit (player feet)
    int chainTop    = (int)yPos + topSpr->getHeight();
    int chainBottom = (int)yInit;
    int tileH       = chainSpr->getHeight();

    for (int tileY = chainTop; tileY < chainBottom; tileY += tileH)
    {
        if (tileY + tileH <= chainBottom)
        {
            graph->draw(chainSpr, (int)xPos, tileY);
        }
        else
        {
            int visibleHeight = chainBottom - tileY;
            if (visibleHeight > 0)
                graph->drawClipped(chainSpr, (int)xPos, tileY, visibleHeight);
        }
    }
}

/**
 * Collision box covers the full canvas width from tip/head top down to yInit.
 */
CollisionBox ClawShot::getCollisionBox() const
{
    int w = clawAnim ? clawAnim->getWidth() : 0;
    return { (int)xPos, (int)yPos, w, (int)yInit - (int)yPos };
}

/**
 * Ball hit: free the shot slot and remove the claw immediately.
 */
void ClawShot::onBallHit(Ball* b)
{
    player->looseShoot();
    kill();
}

/**
 * Floor hit: stick the claw to the top of the platform.
 */
void ClawShot::onFloorHit(Platform* f)
{
    if (stuck) return;  // Prevent re-entry each frame while already stuck

    stuck = true;
    stuckTimer = STUCK_DURATION;

    // Snap to the top of the platform so the claw head sits on the surface
    if (f)
    {
        CollisionBox floorBox = f->getCollisionBox();
        yPos = static_cast<float>(floorBox.y);
    }

    LOG_TRACE("ClawShot stuck to platform at y=%.1f", yPos);
}

/**
 * Ceiling hit: stick the claw to the top of the play area.
 */
void ClawShot::onCeilingHit()
{
    if (stuck) return;

    stuck = true;
    stuckTimer = STUCK_DURATION;
    yPos = static_cast<float>(Stage::MIN_Y);

    LOG_TRACE("ClawShot stuck to ceiling at y=%.1f", yPos);
}
