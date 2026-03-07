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
public:
    virtual ~Platform() = default;

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    virtual void update(float dt) {}

    /**
     * @brief Called when a weapon shot hits this platform.
     * Default no-op; Glass overrides to start destruction animation.
     */
    virtual void onHit() {}

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

    CollisionBox getCollisionBox() const override
    {
        return { (int)xPos, (int)yPos, getWidth(), getHeight() };
    }
};
