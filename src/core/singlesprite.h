#pragma once

#include "isprite.h"

class Sprite;

/**
 * SingleSprite - Holds a single non-owning sprite reference
 *
 * Use for entities with a single static or externally-managed sprite:
 * Ball, Floor, Ladder
 *
 * Usage:
 *   SingleSprite sprite;
 *   sprite.setSprite(&gameinf.getStageRes().redball[size]);
 *   Sprite* spr = sprite.getActiveSprite();
 */
class SingleSprite : public ISprite
{
private:
    Sprite* sprite = nullptr;

public:
    SingleSprite() = default;
    explicit SingleSprite(Sprite* spr) : sprite(spr) {}

    /**
     * Set the sprite reference (non-owning)
     * @param spr Pointer to sprite, or nullptr to clear
     */
    void setSprite(Sprite* spr) { sprite = spr; }

    /**
     * Get the sprite reference
     */
    Sprite* getSprite() const { return sprite; }

    /**
     * ISprite interface implementation
     */
    Sprite* getActiveSprite() const override { return sprite; }
};
