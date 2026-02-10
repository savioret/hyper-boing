#include "main.h"
#include "harpoonshot.h"
#include "../game/scene.h"
#include "player.h"
#include "logger.h"
#include "../core/graph.h"
#include "../core/sprite.h"
#include "../core/spritesheet.h"

/**
 * HarpoonShot constructor
 *
 * Initializes a harpoon/chain projectile with tail animation.
 *
 * @param scn The scene this shot belongs to
 * @param pl The player who fired the shot
 * @param type The weapon type (HARPOON)
 */
HarpoonShot::HarpoonShot(Scene* scn, Player* pl, WeaponType type)
    : Shot(scn, pl, type)
    //, tailAnim(std::make_unique<ToggleAnim>(0, 1, 33))  // Toggle between 0 and 1 every 33ms (2 frames at 60fps)
{
    // Get harpoon sprites from shared resources in AppData
    StageResources& res = gameinf.getStageRes();
    tipSprite = &res.harpoonTip;
    chainSpriteSheet = &res.harpoonChain;

    // Clone the animation template from AppData (each shot gets its own independent animation)
    tailAnim = res.harpoonAnim->clone();

    // Coordinate system:
    // - yInit = player's feet Y (chain bottom anchor, never changes)
    // - yPos = harpoon head top-left (moves upward during flight)
    //
    // Initially, position the head so its bottom aligns with player's feet.
    // This means yPos = yInit - headHeight
    yPos -= tipSprite->getHeight();

    // When climbing, center the harpoon head on the player
    if (pl->isClimbing())
    {
        xPos = xInit = xInit - tipSprite->getWidth() / 2.0f;
    }
    else
    {
        int off = 10;
        if ( pl->getFacing() == FacingDirection::LEFT )
        {
            xPos = xInit = pl->getX() - tipSprite->getWidth()/2.0f - off;
        }
        else  // FacingDirection::RIGHT
        {
            xPos = xInit = pl->getX() - tipSprite->getWidth() / 2.0f + off;
        }
    }
}

HarpoonShot::~HarpoonShot()
{
}

/**
 * Update harpoon movement and tail animation
 *
 * Moves the harpoon upward and toggles the tail animation sprite.
 * When reaching the top of the screen, triggers ceiling hit behavior.
 *
 * @param dt Delta time in seconds
 */
void HarpoonShot::update(float dt)
{
    // Convert dt from seconds to milliseconds for animation controller
    float dtMs = dt * 1000.0f;
    tailAnim->update(dtMs);

    if (!isDead())
    {
        // Check if reached top of screen
        if (yPos <= MIN_Y)
        {
            onCeilingHit();  // Default: player->looseShoot() + kill()
        }
        else
        {
            yPos -= weaponSpeed;  // Move upward
        }
    }
}

/**
 * Draw the harpoon chain
 *
 * Renders the tip sprite at the current position, then draws the animated
 * chain extending downward to the anchor point (yInit = player's feet).
 * The last tile is clipped to not extend past yInit.
 */
void HarpoonShot::draw(Graph* graph)
{
    // Draw tip sprite (yPos already accounts for sprite height offset)
    graph->draw(tipSprite, (int)xPos, (int)yPos);

    // Draw animated chain from tip bottom down to the anchor point (yInit)
    int frameIndex = tailAnim->getCurrentFrame();
    Sprite* chainFrame = chainSpriteSheet->getFrame(frameIndex);
    LOG_DEBUG("Harpoon tailAnim frame: %d", frameIndex);
    if (!chainFrame) return;  // Safety check

    int tileHeight = chainFrame->getHeight();
    int chainTop = (int)yPos + tipSprite->getHeight();
    int chainBottom = (int)yInit;

    for (int tileY = chainTop; tileY < chainBottom; tileY += tileHeight)
    {
        // Check if this tile would extend past the anchor point
        if (tileY + tileHeight <= chainBottom)
        {
            // Full tile fits - draw normally
            graph->draw(chainFrame, (int)xPos, tileY);
        }
        else
        {
            // Last tile would extend past anchor - draw clipped
            // Use SDL_RenderCopy with a source rect to clip the sprite
            int visibleHeight = chainBottom - tileY;
            if (visibleHeight > 0)
            {
                graph->drawClipped(chainFrame, (int)xPos, tileY, visibleHeight);
            }
        }
    }
}

/**
 * Get collision box for the harpoon chain
 *
 * The harpoon has a chain that extends from the tip down to the bottom
 * of the screen (MAX_Y). The collision box covers this entire vertical area.
 */
CollisionBox HarpoonShot::getCollisionBox() const
{
    int width = tipSprite->getWidth();
    int height = (int)yInit - (int)yPos;
    return { (int)xPos, (int)yPos, width, height };
}
