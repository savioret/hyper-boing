#pragma once

#include "shot.h"

class Sprite;

/**
 * HarpoonShot - Chain weapon projectile
 *
 * Fires a vertical chain that extends from the projectile head down to the
 * bottom of the screen. The tail animates by alternating between two sprite
 * variants.
 *
 * This preserves the original HARPOON weapon behavior from the Shoot class.
 */
class HarpoonShot : public Shot
{
private:
    Sprite* sprites[3];  // Head + 2 tail variants
    int tail;            // 0 or 1 (toggles between tail sprites)
    int tailCounter;
    int tailDelay;

public:
    /**
     * Constructor
     * @param scn Scene this shot belongs to
     * @param pl Player who fired the shot
     * @param type Weapon type (HARPOON or HARPOON2)
     * @param xOffset Horizontal offset for multi-projectile weapons
     */
    HarpoonShot(Scene* scn, Player* pl, WeaponType type, int xOffset);
    ~HarpoonShot();

    // Implement abstract interface from Shot
    void move() override;
    void draw(Graph* graph) override;

    // Accessors for Scene compatibility
    Sprite* getSprite(int index) const { return sprites[index]; }
    int getTail() const { return tail; }
};
