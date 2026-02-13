#pragma once

class Sprite;

/**
 * ISprite - Interface for objects that provide a renderable sprite
 *
 * Provides a unified way to get the "active" sprite for rendering,
 * regardless of whether it's a single sprite, animated sprite, or
 * complex multi-animation system.
 *
 * Implementations:
 * - SingleSprite: Holds single Sprite* (for Ball, Floor, Ladder)
 * - AnimSprite: Holds SpriteSheet + IAnimController (for GunShot)
 * - MultiAnimSprite: Manages multiple named AnimSpriteSheets (for Player)
 */
class ISprite
{
public:
    virtual ~ISprite() = default;

    /**
     * Get the currently active sprite for rendering
     * @return Pointer to current Sprite, or nullptr if none
     */
    virtual Sprite* getActiveSprite() const = 0;

    /**
     * Check if this holder has a valid sprite to render
     */
    virtual bool hasSprite() const { return getActiveSprite() != nullptr; }

    /**
     * Get width of current sprite
     * @return Width in pixels, or 0 if no sprite
     */
    int getWidth() const;

    /**
     * Get height of current sprite
     * @return Height in pixels, or 0 if no sprite
     */
    int getHeight() const;
};
