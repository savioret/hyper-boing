#pragma once

#include "shot.h"
#include "../core/singlesprite.h"
#include "../core/animsprite.h"

/**
 * HarpoonShot - Chain weapon projectile
 *
 * Fires a vertical chain that extends from the projectile head down to the
 * bottom of the screen. The tail animates by alternating between two sprite
 * variants using a ToggleAnim controller.
 *
 * This preserves the original HARPOON weapon behavior from the Shoot class.
 */
class HarpoonShot : public Shot
{
private:
    SingleSprite tipSpr;         // Harpoon tip
    AnimSprite chainSpr;         // Animated chain

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
     * @brief Get collision box for the harpoon chain (ball/hexa collision)
     * @return Collision box covering the entire vertical chain from tip to feet
     */
    CollisionBox getCollisionBox() const override;

    /**
     * @brief Get collision box for floor/ceiling collision only
     * @return Collision box from tip down to gun position (head level)
     *
     * This prevents the chain from colliding with platforms that the
     * player's head is above but feet are below (e.g., climbing through).
     */
    CollisionBox getFloorCollisionBox() const override;
};
