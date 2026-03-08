#pragma once

#include "gameobject.h"

// Forward declarations
class Sprite;

/**
 * Platform class
 *
 * Abstract base class for solid platforms that balls bounce off,
 * players walk on, and weapons stop at.
 *
 * Subclasses: Floor (static platform), Glass (destructible platform)
 */
class Platform : public IGameObject
{
protected:
    bool invisible   = false;  ///< Hidden until revealed by a weapon shot
    bool passthrough = false;  ///< Player can walk through horizontally (default: blocking)

public:
    virtual ~Platform() = default;

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    virtual void update(float dt) {}

    /**
     * @brief Called when a weapon shot hits this platform.
     * Base implementation clears the invisible flag (reveals the platform).
     * Subclasses should call Platform::onHit() before their own logic.
     */
    virtual void onHit() { invisible = false; }

    /**
     * @brief Returns true if this platform is destructible (e.g., Glass).
     * Destructible platforms use full shot collision box (like balls),
     * while regular floors use reduced floor collision box.
     */
    virtual bool isDestructible() const { return false; }

    /**
     * @brief Returns the sprite to render for this platform's current state.
     */
    virtual Sprite* getCurrentSprite() const = 0;

    bool isInvisible()   const { return invisible; }
    void setInvisible(bool v)  { invisible = v; }

    bool isPassthrough() const { return passthrough; }
    void setPassthrough(bool v){ passthrough = v; }

    CollisionBox getCollisionBox() const override
    {
        return { (int)xPos, (int)yPos, getWidth(), getHeight() };
    }
};
