#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

/**
 * IAnimController - Interface for animation controllers
 *
 * Animation controllers manage frame selection based on elapsed time/ticks.
 * They are composable into entities without inheritance, allowing flexible
 * animation management.
 *
 * Usage:
 *   auto anim = std::make_unique<FrameSequenceAnim>({0, 1, 2, 3}, 10, true);
 *   anim->update();  // Call each frame
 *   int frame = anim->getCurrentFrame();
 */
class IAnimController
{
public:
    virtual ~IAnimController() = default;

    /**
     * Update animation state
     * @param dt Delta time (typically 1.0f for frame-based, or actual dt for time-based)
     */
    virtual void update(float dt = 1.0f) = 0;

    /**
     * Get current frame index
     */
    virtual int getCurrentFrame() const = 0;

    /**
     * Reset animation to initial state
     */
    virtual void reset() = 0;

    /**
     * Check if animation has completed (for non-looping animations)
     */
    virtual bool isComplete() const { return false; }
};

/**
 * FrameSequenceAnim - Plays through a sequence of frame indices
 *
 * Supports:
 * - Linear playback (0, 1, 2, 3, 4...)
 * - Custom sequences (0, 1, 2, 1, 0 for ping-pong)
 * - Looping or one-shot
 * - Completion callback
 *
 * Example:
 *   // Walk animation: frames 0-4, 10 ticks per frame, looping
 *   FrameSequenceAnim walkAnim({0, 1, 2, 3, 4}, 10, true);
 *
 *   // Or use convenience constructor:
 *   auto anim = FrameSequenceAnim::range(0, 4, 10, true);
 */
class FrameSequenceAnim : public IAnimController
{
private:
    std::vector<int> frames;           // Frame indices to play
    std::vector<int> frameDurations;   // Duration in milliseconds for each frame (if per-frame)
    int currentIndex = 0;              // Index into frames vector
    int defaultDuration;               // Default duration in milliseconds
    float timeAccumulator = 0.0f;      // Accumulated time in milliseconds
    bool loop = true;
    bool complete = false;
    bool usePerFrameDurations = false; // Whether to use per-frame durations
    std::function<void()> onComplete;

public:
    /**
     * Create a sequential animation with uniform duration
     * @param frameSequence Vector of frame indices to play
     * @param durationMs Duration in milliseconds for each frame
     * @param shouldLoop Whether to loop when reaching the end
     */
    FrameSequenceAnim(std::vector<int> frameSequence, int durationMs, bool shouldLoop = true);

    /**
     * Create a sequential animation with per-frame durations
     * @param frameSequence Vector of frame indices to play
     * @param durationsMs Duration in milliseconds for each frame
     * @param shouldLoop Whether to loop when reaching the end
     */
    FrameSequenceAnim(std::vector<int> frameSequence, std::vector<int> durationsMs, bool shouldLoop = true);

    /**
     * Convenience: Create animation from start to end frame (inclusive)
     */
    static FrameSequenceAnim range(int startFrame, int endFrame, int durationMs, bool loop = true);

    /**
     * Convenience: Create looping animation that oscillates between two frames
     */
    static FrameSequenceAnim oscillate(int frameA, int frameB, int durationMs);

    void update(float dt = 1.0f) override;  // dt in milliseconds (typically 16.67ms for 60fps)
    int getCurrentFrame() const override;
    void reset() override;
    bool isComplete() const override { return complete; }

    /**
     * Set callback for when animation completes (non-looping only)
     */
    void setOnComplete(std::function<void()> callback) { onComplete = std::move(callback); }

    /**
     * Get the default frame duration in milliseconds
     */
    int getDefaultDuration() const { return defaultDuration; }

    /**
     * Set the default frame duration in milliseconds
     */
    void setDefaultDuration(int durationMs) { defaultDuration = durationMs; }
};

/**
 * ToggleAnim - Binary toggle between two frame values
 *
 * Simpler than FrameSequenceAnim for cases like HarpoonShot tail animation
 * where we just need to alternate between two values.
 *
 * Example:
 *   ToggleAnim tailAnim(0, 1, 2);  // Toggle between 0 and 1 every 2 ticks
 */
class ToggleAnim : public IAnimController
{
private:
    int frameA, frameB;
    int currentFrame;
    int toggleDuration;              // Duration in milliseconds
    float timeAccumulator = 0.0f;    // Accumulated time in milliseconds

public:
    /**
     * Create a toggle animation
     * @param a First frame value
     * @param b Second frame value
     * @param durationMs Duration in milliseconds between toggles
     */
    ToggleAnim(int a, int b, int durationMs);

    void update(float dt = 1.0f) override;  // dt in milliseconds
    int getCurrentFrame() const override { return currentFrame; }
    void reset() override;
};

/**
 * StateMachineAnim - Animation with named states
 *
 * Each state has its own frame sequence. Transitions are explicit via setState().
 * States can auto-transition to another state when complete.
 *
 * Example:
 *   StateMachineAnim anim;
 *   anim.addState("flight", {0, 1, 2, 3, 4}, 9, false, "flight_loop");
 *   anim.addState("flight_loop", {3, 4}, 9, true);
 *   anim.addState("impact", {5, 6}, 9, false);
 *   anim.setState("flight");
 *
 *   // When "flight" completes, it auto-transitions to "flight_loop"
 *   // Call anim.setState("impact") to trigger impact animation
 */
class StateMachineAnim : public IAnimController
{
public:
    struct State
    {
        std::vector<int> frames;           // Frame sequence for this state
        std::vector<int> frameDurations;   // Duration in milliseconds for each frame (if per-frame)
        int defaultDuration;               // Default duration in milliseconds
        bool loop;                         // Whether to loop
        bool usePerFrameDurations;         // Whether to use per-frame durations
        std::string nextState;             // Auto-transition to this state when complete (empty = stay)
    };

private:
    std::unordered_map<std::string, State> states;
    std::string currentStateName;
    int currentIndex = 0;
    float timeAccumulator = 0.0f;  // Accumulated time in milliseconds
    bool stateComplete = false;
    std::function<void(const std::string&)> onStateComplete;

public:
    StateMachineAnim() = default;

    /**
     * Add a state with uniform frame duration
     * @param name State name (used for transitions)
     * @param frames Frame sequence
     * @param durationMs Duration in milliseconds for each frame
     * @param loop Whether to loop
     * @param nextState Auto-transition target when complete (empty = stay in state)
     */
    void addState(const std::string& name, std::vector<int> frames, int durationMs,
                  bool loop = true, const std::string& nextState = "");

    /**
     * Add a state with per-frame durations
     * @param name State name (used for transitions)
     * @param frames Frame sequence
     * @param durationsMs Duration in milliseconds for each frame
     * @param loop Whether to loop
     * @param nextState Auto-transition target when complete (empty = stay in state)
     */
    void addState(const std::string& name, std::vector<int> frames, std::vector<int> durationsMs,
                  bool loop = true, const std::string& nextState = "");

    /**
     * Transition to a different state
     */
    void setState(const std::string& name);

    /**
     * Get current state name
     */
    const std::string& getStateName() const { return currentStateName; }

    /**
     * Check if current state has completed (non-looping states)
     */
    bool isStateComplete() const { return stateComplete; }

    void update(float dt = 1.0f) override;  // dt in milliseconds
    int getCurrentFrame() const override;
    void reset() override;
    bool isComplete() const override { return stateComplete; }

    /**
     * Callback when a non-looping state completes
     */
    void setOnStateComplete(std::function<void(const std::string&)> callback)
    {
        onStateComplete = std::move(callback);
    }
};
