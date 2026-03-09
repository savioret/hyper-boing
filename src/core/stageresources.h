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
    // Ball sprites per color (0=red, 1=green, 2=blue)
    static constexpr int BALL_COLOR_COUNT = 3;
    std::unique_ptr<AnimSpriteSheet> ballAnim[BALL_COLOR_COUNT];
    std::unique_ptr<AnimSpriteSheet> ballSplashAnim[BALL_COLOR_COUNT];

    // Floor sprite sheets per color (0=red, 1=blue, 2=green, 3=yellow)
    static constexpr int FLOOR_COLOR_COUNT = 4;
    std::unique_ptr<AnimSpriteSheet> floorAnim[FLOOR_COLOR_COUNT];

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

    // Glass platform sprite sheets per color (0=red, 1=blue, 2=green, 3=yellow)
    static constexpr int GLASS_COLOR_COUNT = 4;
    std::unique_ptr<AnimSpriteSheet> glassAnim[GLASS_COLOR_COUNT];

    // Hexa enemy sprites per color (0=green, 1=cyan, 2=orange, 3=purple)
    static constexpr int HEXA_COLOR_COUNT = 4;
    std::unique_ptr<AnimSpriteSheet> hexaAnim[HEXA_COLOR_COUNT];
    std::unique_ptr<AnimSpriteSheet> hexaSplashAnim[HEXA_COLOR_COUNT];

    bool initialized;      ///< True if resources have been loaded

    StageResources() : initialized(false) {}

    AnimSpriteSheet* getBallAnim(int color) const
    {
        int c = (color >= 0 && color < BALL_COLOR_COUNT) ? color : 0;
        return ballAnim[c].get();
    }
    AnimSpriteSheet* getBallSplashAnim(int color) const
    {
        int c = (color >= 0 && color < BALL_COLOR_COUNT) ? color : 0;
        return ballSplashAnim[c].get();
    }
    AnimSpriteSheet* getHexaAnim(int color) const
    {
        int c = (color >= 0 && color < HEXA_COLOR_COUNT) ? color : 0;
        return hexaAnim[c].get();
    }
    AnimSpriteSheet* getHexaSplashAnim(int color) const
    {
        int c = (color >= 0 && color < HEXA_COLOR_COUNT) ? color : 0;
        return hexaSplashAnim[c].get();
    }
    AnimSpriteSheet* getFloorAnim(int color) const
    {
        int c = (color >= 0 && color < FLOOR_COLOR_COUNT) ? color : 0;
        return floorAnim[c].get();
    }
    AnimSpriteSheet* getGlassAnim(int color) const
    {
        int c = (color >= 0 && color < GLASS_COLOR_COUNT) ? color : 0;
        return glassAnim[c].get();
    }
};
