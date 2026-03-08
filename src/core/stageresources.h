#pragma once

#include <memory>
#include "sprite.h"
#include "spritesheet.h"
#include "animspritesheet.h"
#include "animcontroller.h"

// Forward declarations
class Sprite;

/**
 * @struct StageResources
 * @brief Shared sprites used across all stages
 *
 * These resources are loaded once at startup and reused by all scenes,
 * avoiding redundant loading/unloading between stage transitions.
 */
struct StageResources
{
    // Ball sprites (single spritesheet with 4 size frames)
    std::unique_ptr<AnimSpriteSheet> ballAnim;        ///< Ball sprite sheet (4 sizes as frames)
    std::unique_ptr<AnimSpriteSheet> ballSplashAnim;  ///< Ball splash animation (single shared)

    // Floor sprite sheet (shared; instances index frames directly)
    std::unique_ptr<AnimSpriteSheet> floorBricksAnim; ///< Floor bricks sprite sheet (5 variants)

    // Ladder sprite
    Sprite ladder;      ///< Ladder tile sprite (tiled vertically)

    // Weapon sprites
    Sprite harpoonTip;                        ///< Harpoon tip sprite
    SpriteSheet harpoonChain;                 ///< Harpoon chain animated sprite sheet (shared texture)
    std::unique_ptr<IAnimController> harpoonAnim;       ///< Template animation for harpoon chain (cloned per shot)
    std::unique_ptr<AnimSpriteSheet> gunBulletAnim;     ///< Gun bullet sprite sheet with animation (cloned per shot)
    std::unique_ptr<AnimSpriteSheet> clawWeaponAnim;       ///< Claw weapon sprite sheet (3 frames; cloned per shot)
    std::unique_ptr<AnimSpriteSheet> clawWeaponYellowAnim; ///< Yellow claw skin (2 frames: head+chain; shown in last second)

    // Effect animation templates (cloned per instance)
    std::unique_ptr<AnimSpriteSheet> gunSparkAnim;     ///< Gun muzzle flash effect
    std::unique_ptr<AnimSpriteSheet> harpoonSparkAnim; ///< Harpoon muzzle flash effect

    // Border/marker sprites
    Sprite mark[5];     ///< Border marker sprites

    // UI sprites
    Sprite miniplayer[2];  ///< Mini player icons for HUD
    Sprite lives[2];       ///< Lives icons for HUD
    Sprite time;           ///< Time icon
    Sprite gameover;       ///< Game over text
    Sprite continu;        ///< Continue text
    Sprite ready;          ///< Ready text

    // Font sprites
    Sprite fontnum[3];     ///< Number fonts (3 sizes)

    // Pickup sprites
    Sprite pickupSprites[6];                            ///< Pickup sprites (SHIELD excluded; CLAW at index 5)
    std::unique_ptr<AnimSpriteSheet> pickupShieldAnim;  ///< Shield pickup animated spritesheet
    Sprite itemHolder;                                  ///< Item holder box (42x42) drawn behind active weapon icon

    // Player shield effect animation template (cloned per player)
    std::unique_ptr<AnimSpriteSheet> shieldAnim;  ///< Player shield effect animation

    // Glass platform sprite sheet (shared; instances index frames directly)
    std::unique_ptr<AnimSpriteSheet> glassBricksAnim;  ///< Glass bricks sprite sheet

    // Hexa enemy sprites
    std::unique_ptr<AnimSpriteSheet> hexaAnim;         ///< Hexa sprite sheet (3 sizes as frames)
    std::unique_ptr<AnimSpriteSheet> hexaSplashAnim;   ///< Hexa death splash animation

    bool initialized;      ///< True if resources have been loaded

    StageResources() : initialized(false) {}
};
