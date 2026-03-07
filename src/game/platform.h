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
     * Default no-op; Glass overrides to advance its damage state.
     */
    virtual void onHit() {}

    /**
     * @brief Returns the sprite to render for this platform's current state.
     */
    virtual Sprite* getCurrentSprite() const = 0;

    CollisionBox getCollisionBox() const override
    {
        return { (int)xPos, (int)yPos, getWidth(), getHeight() };
    }
};
