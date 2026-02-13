#pragma once

#include "isprite.h"
#include <unordered_map>
#include <string>
#include <vector>

class AnimSpriteSheet;
class Sprite;

/**
 * MultiAnimSprite - Manages multiple named animations with fallback
 *
 * Use for entities with multiple animation states: Player.
 * Replaces the complex Sprite2D + AnimSpriteSheet combination.
 *
 * The AnimSpriteSheets are non-owning references - the entity owns them.
 * This class just provides a unified way to switch between them.
 *
 * Usage:
 *   MultiAnimSprite sprite;
 *   sprite.registerAnimation("walk", walkAnim.get());
 *   sprite.registerAnimation("victory", victoryAnim.get());
 *   sprite.setFallbackSprite(idleSprite);
 *
 *   // In setState():
 *   sprite.setActiveAnimation("walk");
 *
 *   // In update():
 *   sprite.update(dtMs);
 *
 *   // In draw():
 *   Sprite* spr = sprite.getActiveSprite();
 */
class MultiAnimSprite : public ISprite
{
private:
    // Named animations (non-owning references)
    std::unordered_map<std::string, AnimSpriteSheet*> animations;

    // Fallback sprites for states without animation (e.g., IDLE, SHOOT, DEAD)
    std::vector<Sprite*> fallbackSprites;
    int fallbackFrame = 0;

    // Currently active animation key
    std::string activeAnimKey;
    bool useFallback = true;

public:
    MultiAnimSprite() = default;
    ~MultiAnimSprite() = default;

    /**
     * Register a named animation (non-owning reference)
     * @param name Animation name (e.g., "walk", "victory", "climb")
     * @param anim Pointer to AnimSpriteSheet owned by entity
     */
    void registerAnimation(const std::string& name, AnimSpriteSheet* anim);

    /**
     * Unregister a named animation
     */
    void unregisterAnimation(const std::string& name);

    /**
     * Clear all registered animations
     */
    void clearAnimations();

    /**
     * Add a fallback sprite (for states without animation)
     * @param spr Non-owning pointer to sprite
     */
    void addFallbackSprite(Sprite* spr);

    /**
     * Set all fallback sprites at once
     */
    void setFallbackSprites(const std::vector<Sprite*>& sprites);

    /**
     * Clear all fallback sprites
     */
    void clearFallbackSprites();

    /**
     * Set current fallback frame index
     */
    void setFallbackFrame(int frame);

    /**
     * Get current fallback frame index
     */
    int getFallbackFrame() const { return fallbackFrame; }

    /**
     * Activate a named animation
     * @param name Animation name
     * @return true if animation exists and was activated
     */
    bool setActiveAnimation(const std::string& name);

    /**
     * Switch to using fallback sprite instead of animation
     */
    void useAsFallback();

    /**
     * Check if currently using fallback sprite
     */
    bool isUsingFallback() const { return useFallback; }

    /**
     * Get current animation key (empty if using fallback)
     */
    const std::string& getActiveAnimKey() const { return activeAnimKey; }

    /**
     * Update active animation (if not using fallback)
     * @param dtMs Delta time in milliseconds
     */
    void update(float dtMs);

    /**
     * ISprite interface implementation
     */
    Sprite* getActiveSprite() const override;

    /**
     * Get the currently active AnimSpriteSheet (nullptr if using fallback)
     */
    AnimSpriteSheet* getActiveAnimation() const;

    /**
     * Get animation by name (nullptr if not found)
     */
    AnimSpriteSheet* getAnimation(const std::string& name) const;
};
