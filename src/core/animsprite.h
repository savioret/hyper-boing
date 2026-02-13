#pragma once

#include "isprite.h"
#include <memory>
#include <string>

class SpriteSheet;
class IAnimController;
class StateMachineAnim;
class Sprite;

/**
 * AnimSprite - Holds an animated sprite (SpriteSheet + animation controller)
 *
 * Use for entities with simple animation: GunShot, single-animation effects.
 * The SpriteSheet is non-owning (typically owned by AppData or Scene).
 * The animation controller is owned (typically cloned per instance).
 *
 * Usage:
 *   AnimSprite sprite;
 *   sprite.init(sheet, clonedAnim);
 *   sprite.update(dtMs);
 *   Sprite* spr = sprite.getActiveSprite();
 */
class AnimSprite : public ISprite
{
private:
    SpriteSheet* spriteSheet = nullptr;  // Non-owning
    std::unique_ptr<IAnimController> animController;
    StateMachineAnim* stateMachinePtr = nullptr;  // Cached for state features

public:
    AnimSprite() = default;
    ~AnimSprite();

    // Disable copy
    AnimSprite(const AnimSprite&) = delete;
    AnimSprite& operator=(const AnimSprite&) = delete;

    // Enable move
    AnimSprite(AnimSprite&& other) noexcept;
    AnimSprite& operator=(AnimSprite&& other) noexcept;

    /**
     * Initialize with sprite sheet and animation controller
     * @param sheet Non-owning pointer to SpriteSheet
     * @param ctrl Animation controller (ownership transferred)
     */
    void init(SpriteSheet* sheet, std::unique_ptr<IAnimController> ctrl);

    /**
     * Update animation state
     * @param dtMs Delta time in milliseconds
     */
    void update(float dtMs);

    /**
     * Reset animation to initial state
     */
    void reset();

    /**
     * ISprite interface implementation
     */
    Sprite* getActiveSprite() const override;

    /**
     * Get current frame index
     */
    int getCurrentFrame() const;

    /**
     * Check if animation has completed (for non-looping animations)
     */
    bool isComplete() const;

    /**
     * Direct access to animation controller
     */
    IAnimController* getController() { return animController.get(); }
    const IAnimController* getController() const { return animController.get(); }

    /**
     * Direct access to sprite sheet
     */
    SpriteSheet* getSpriteSheet() { return spriteSheet; }
    const SpriteSheet* getSpriteSheet() const { return spriteSheet; }

    // StateMachineAnim-specific features (safe to call on any type)

    /**
     * Check if using StateMachineAnim
     */
    bool hasStates() const { return stateMachinePtr != nullptr; }

    /**
     * Set animation state (only works if StateMachineAnim)
     * @return true if successful
     */
    bool setState(const std::string& stateName);

    /**
     * Get current state name (empty if not StateMachineAnim)
     */
    const std::string& getStateName() const;

    /**
     * Check if current state is complete
     */
    bool isStateComplete() const;

private:
    void updateStateMachineCache();
};
