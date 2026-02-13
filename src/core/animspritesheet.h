#pragma once

#include <memory>
#include <string>
#include <functional>
#include "spritesheet.h"
#include "animcontroller.h"

// Forward declarations
class Graph;

/**
 * AnimSpriteSheet - Combines SpriteSheet with animation controller
 *
 * Provides a unified interface for animated sprites loaded from Aseprite JSON.
 * Encapsulates both the texture/frame data (SpriteSheet) and playback logic
 * (IAnimController) with a clean API.
 *
 * The animation controller can be either StateMachineAnim or FrameSequenceAnim,
 * determined automatically by the JSON structure (presence of frameTags).
 * StateMachineAnim-specific features are safely callable on either type.
 *
 * Ownership models:
 * - OWNED: AnimSpriteSheet owns both texture and animation (default for loading)
 * - SHARED: AnimSpriteSheet uses shared texture, owns independent animation (for cloning)
 *
 * Usage:
 *   // Load from Aseprite JSON
 *   auto walkAnim = AnimSpriteSheet::load(&graph, "assets/graph/players/walk.json");
 *
 *   // Game loop
 *   walkAnim->update(dtMs);
 *   Sprite* sprite = walkAnim->getCurrentSprite();
 *   graph->draw(sprite, x, y);
 *
 *   // For StateMachineAnim features (safe on any type)
 *   if (walkAnim->hasStates()) {
 *       walkAnim->setState("run");
 *   }
 */
class AnimSpriteSheet
{
public:
    // ========================================================================
    // Construction / Factory Methods
    // ========================================================================

    /**
     * Default constructor - creates empty AnimSpriteSheet
     */
    AnimSpriteSheet();

    /**
     * Destructor
     */
    ~AnimSpriteSheet();

    // Disable copy (use clone() for independent copies)
    AnimSpriteSheet(const AnimSpriteSheet&) = delete;
    AnimSpriteSheet& operator=(const AnimSpriteSheet&) = delete;

    // Enable move semantics
    AnimSpriteSheet(AnimSpriteSheet&& other) noexcept;
    AnimSpriteSheet& operator=(AnimSpriteSheet&& other) noexcept;

    /**
     * Load from Aseprite JSON file (primary factory method)
     * Auto-detects animation type: JSON with frameTags -> StateMachineAnim, without -> FrameSequenceAnim
     *
     * @param graph Graphics context for texture loading
     * @param jsonPath Path to Aseprite JSON file
     * @return Loaded AnimSpriteSheet, or nullptr on failure
     */
    static std::unique_ptr<AnimSpriteSheet> load(Graph* graph, const std::string& jsonPath);

    /**
     * Load as StateMachineAnim (typed factory method)
     * Guarantees StateMachineAnim animation type, creates default state if no frameTags
     *
     * @param graph Graphics context for texture loading
     * @param jsonPath Path to Aseprite JSON file
     * @return Loaded AnimSpriteSheet with StateMachineAnim, or nullptr on failure
     */
    static std::unique_ptr<AnimSpriteSheet> loadAsStateMachine(Graph* graph, const std::string& jsonPath);

    // ========================================================================
    // Core Animation API
    // ========================================================================

    /**
     * Update animation state
     * @param dtMs Delta time in milliseconds
     */
    void update(float dtMs);

    /**
     * Get sprite for current animation frame
     * @return Pointer to current Sprite, or nullptr if invalid
     */
    Sprite* getCurrentSprite() const;

    /**
     * Get current frame index
     */
    int getCurrentFrame() const;

    /**
     * Reset animation to initial state
     */
    void reset();

    /**
     * Check if animation has completed (for non-looping animations)
     */
    bool isComplete() const;

    // ========================================================================
    // StateMachineAnim-Specific Features
    // ========================================================================

    /**
     * Check if this AnimSpriteSheet uses StateMachineAnim
     * (required for setState/getStateName/setOnStateComplete)
     */
    bool hasStates() const { return stateMachinePtr != nullptr; }

    /**
     * Transition to a different animation state
     * @param stateName Name of the state to transition to
     * @return true if transition succeeded, false if not a state machine or state not found
     */
    bool setState(const std::string& stateName);

    /**
     * Get current state name
     * @return Current state name, or empty string if not a state machine
     */
    const std::string& getStateName() const;

    /**
     * Check if current state has completed (non-looping states)
     */
    bool isStateComplete() const;

    /**
     * Set callback for when a non-looping state completes
     * @param callback Function called with completed state name
     */
    void setOnStateComplete(std::function<void(const std::string&)> callback);

    // ========================================================================
    // Cloning (for shared texture, independent animation)
    // ========================================================================

    /**
     * Create a clone with independent animation state but shared texture
     *
     * Use case: Multiple entities (e.g., bullets) that share the same texture
     * but need independent animation playback.
     *
     * @return New AnimSpriteSheet with shared texture ownership
     */
    std::unique_ptr<AnimSpriteSheet> clone() const;

    // ========================================================================
    // Direct Access (for advanced use cases)
    // ========================================================================

    /**
     * Get underlying SpriteSheet
     */
    SpriteSheet* getSpriteSheet() const;

    /**
     * Get underlying animation controller
     */
    IAnimController* getAnimController() const { return animController.get(); }

    /**
     * Get frame count from sprite sheet
     */
    int getFrameCount() const;

    /**
     * Get specific frame by index
     * @param index Frame index (0-based)
     * @return Pointer to Sprite, or nullptr if invalid
     */
    Sprite* getFrame(int index) const;

    /**
     * Check if AnimSpriteSheet is valid (has both sheet and animation)
     */
    bool isValid() const;

private:
    // Texture/frame data
    std::unique_ptr<SpriteSheet> ownedSheet;  // Used when owning
    SpriteSheet* sharedSheet;                  // Used when sharing (non-owning)
    bool ownsSheet;

    // Animation controller (always owned)
    std::unique_ptr<IAnimController> animController;

    // Cached pointer to StateMachineAnim for state-based features (nullptr if FrameSequenceAnim)
    StateMachineAnim* stateMachinePtr;

    // Empty string for getStateName() when not a state machine
    static const std::string emptyString;

    /**
     * Internal constructor for owned sheet
     */
    AnimSpriteSheet(std::unique_ptr<SpriteSheet> sheet,
                    std::unique_ptr<IAnimController> anim);

    /**
     * Internal constructor for shared sheet (cloning)
     */
    AnimSpriteSheet(SpriteSheet* sharedSheet,
                    std::unique_ptr<IAnimController> clonedAnim);

    /**
     * Update stateMachinePtr cache after animation change
     */
    void updateStateMachineCache();
};
