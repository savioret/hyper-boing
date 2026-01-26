#include "main.h"
#include "harpoonshot.h"
#include "../game/scene.h"
#include "player.h"
#include "../core/graph.h"
#include "../core/sprite.h"

/**
 * HarpoonShot constructor
 *
 * Initializes a harpoon/chain projectile with tail animation.
 *
 * @param scn The scene this shot belongs to
 * @param pl The player who fired the shot
 * @param type The weapon type (HARPOON or HARPOON2)
 * @param xOffset Horizontal offset for multi-projectile weapons
 */
HarpoonShot::HarpoonShot(Scene* scn, Player* pl, WeaponType type, int xOffset)
    : Shot(scn, pl, type, xOffset)
    , tailAnim(std::make_unique<ToggleAnim>(0, 1, 2))  // Toggle between 0 and 1 every 2 ticks
{
    // Use scene's harpoon sprites
    // Note: Currently using old shoot[] sprites, will be updated in Scene refactoring
    sprites[0] = &scene->bmp.shoot[0];  // Head
    sprites[1] = &scene->bmp.shoot[1];  // Tail variant 1
    sprites[2] = &scene->bmp.shoot[2];  // Tail variant 2
}

HarpoonShot::~HarpoonShot()
{
}

/**
 * Update harpoon movement and tail animation
 *
 * Moves the harpoon upward and toggles the tail animation sprite.
 * When reaching the top of the screen, triggers ceiling hit behavior.
 */
void HarpoonShot::move()
{
    // Update tail animation
    tailAnim->update();

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
 * Renders the head sprite at the current position, then draws the tail
 * chain extending downward to the bottom of the screen.
 */
void HarpoonShot::draw(Graph* graph)
{
    // Draw head sprite
    graph->draw(sprites[0], (int)xPos, (int)yPos);

    // Draw tail chain from head to bottom of screen
    int tail = tailAnim->getCurrentFrame();
    for (int i = (int)yPos + sprites[0]->getHeight();
         i < MAX_Y;
         i += sprites[1]->getHeight())
    {
        graph->draw(sprites[1 + tail], (int)xPos, i);
    }
}
