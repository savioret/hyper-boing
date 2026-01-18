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

public:
    virtual ~IGameObject() = default;
    
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
