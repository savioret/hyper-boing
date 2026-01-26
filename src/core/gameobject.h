#pragma once

/**
 * IGameObject interface
 * 
 * Base class for game objects that need lifecycle management.
 * Enables safe deferred deletion pattern to avoid iterator invalidation.
 * 
 * Objects marked as "dead" via kill() are removed during the cleanup phase
 * of the game loop, not immediately.
 */
class IGameObject
{
protected:
    bool dead = false;
    float xPos = 0.0f;
    float yPos = 0.0f;

    /**
     * Reset the dead flag. Used by entities that can revive (e.g., Player).
     * Most game objects should not call this - they are deleted when dead.
     */
    void resetDead() { dead = false; }

public:
    virtual ~IGameObject() = default;

    // Position methods - virtual to allow delegation (e.g., Player delegates to Sprite2D)
    virtual void setPos(float x, float y) { xPos = x; yPos = y; }
    virtual void setX(float x) { xPos = x; }
    virtual void setY(float y) { yPos = y; }
    virtual float getX() const { return xPos; }
    virtual float getY() const { return yPos; }
    
    /**
     * Check if this object should be removed from the game world.
     * @return true if the object is marked for deletion
     */
    virtual bool isDead() const { return dead; }
    
    /**
     * Mark this object for deletion.
     * The actual deletion happens during the cleanup phase.
     */
    virtual void kill() 
    {
        if (!dead)
        {
            dead = true;
            onDeath();
        }
    }
    
    /**
     * Optional hook for lifecycle events.
     * Called once when the object is first killed.
     * Override in derived classes to emit events or perform cleanup.
     */
    virtual void onDeath() {}
};
