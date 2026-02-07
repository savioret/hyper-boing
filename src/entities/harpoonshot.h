#pragma once

#include "shot.h"
#include "../core/animcontroller.h"
#include <memory>

class Sprite;

/**
 * HarpoonShot - Chain weapon projectile
 *
 * Fires a vertical chain that extends from the projectile head down to the
 * bottom of the screen. The tail animates by alternating between two sprite
 * variants using a ToggleAnim controller.
 *
 * This preserves the original HARPOON weapon behavior from the Shoot class.
 */
class SpriteSheet;

class HarpoonShot : public Shot
{
private:
    Sprite* tipSprite;              // Harpoon tip
    SpriteSheet* chainSpriteSheet;  // Animated chain sprite sheet
    std::unique_ptr<IAnimController> tailAnim;  // Animation controller for chain (owns the animation)

public:
    /**
     * Constructor
     * @param scn Scene this shot belongs to
     * @param pl Player who fired the shot
     * @param type Weapon type (HARPOON)
     * @param xOffset Horizontal offset for multi-projectile weapons
     */
    HarpoonShot(Scene* scn, Player* pl, WeaponType type);
    ~HarpoonShot();

    // Implement abstract interface from Shot
    void update(float dt) override;
    void draw(Graph* graph) override;

    /**
     * @brief Get collision box for the harpoon chain
     * @return Collision box covering the entire vertical chain from head to screen bottom
     */
    CollisionBox getCollisionBox() const override;
};
