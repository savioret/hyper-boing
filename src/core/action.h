#pragma once

#include "motion.h"
#include <memory>
#include <vector>
#include <functional>

/**
 * Action - Abstract base for time-bound behaviors.
 *
 * Actions encapsulate animated behaviors with a clear lifecycle:
 * start() -> update(dt)* -> stop()
 *
 * Actions receive delta time and return true while running, false when complete.
 */
class Action
{
public:
    virtual ~Action() = default;

    /**
     * Called once when action begins
     */
    virtual void start() {}

    /**
     * Called each frame with delta time
     * @param dt Delta time in seconds
     * @return true to continue, false when done
     */
    virtual bool update(float dt) = 0;

    /**
     * Called once when action ends (natural completion or forced)
     */
    virtual void stop() {}

    /**
     * Force immediate completion
     */
    void finish();

    /**
     * Check if action is finished
     */
    bool isFinished() const { return isDone; }

protected:
    bool isDone = false;
};

/**
 * ActionSequence - Runs actions one after another.
 *
 * Executes actions in the order they were added. Each action must
 * complete before the next one starts.
 *
 * Usage:
 *   auto seq = std::make_unique<ActionSequence>();
 *   seq->then(action1);
 *   seq->then(action2);
 *   seq->start();
 */
class ActionSequence : public Action
{
private:
    std::vector<std::unique_ptr<Action>> actions;
    size_t currentIndex;
    bool started;

public:
    ActionSequence();

    /**
     * Add an action to the sequence (fluent builder)
     */
    ActionSequence& then(std::unique_ptr<Action> action);

    void start() override;
    bool update(float dt) override;
    void stop() override;
};

/**
 * ActionParallel - Runs all actions simultaneously.
 *
 * Completes when ALL child actions are done.
 *
 * Usage:
 *   auto parallel = std::make_unique<ActionParallel>();
 *   parallel->add(action1);
 *   parallel->add(action2);
 *   parallel->start();
 */
class ActionParallel : public Action
{
private:
    std::vector<std::unique_ptr<Action>> actions;
    bool started;

public:
    ActionParallel();

    /**
     * Add an action to run in parallel (fluent builder)
     */
    ActionParallel& add(std::unique_ptr<Action> action);

    void start() override;
    bool update(float dt) override;
    void stop() override;
};

/**
 * TweenAction - Animates a float* using Motion.
 */
class TweenAction : public Action
{
private:
    float* target;
    Motion motion;

public:
    TweenAction(float* target, float endValue, float durationSeconds,
                Easing easing = Easing::Linear,
                int loops = 1, bool swing = false);

    bool update(float dt) override;
};

/**
 * TweenIntAction - Same as TweenAction but for int*.
 *
 * Converts Motion's float value to int.
 */
class TweenIntAction : public Action
{
private:
    int* target;
    Motion motion;

public:
    TweenIntAction(int* target, int endValue, float durationSeconds,
                   Easing easing = Easing::Linear,
                   int loops = 1, bool swing = false);

    bool update(float dt) override;
};

/**
 * Tween2DAction - Animates x and y positions together using Motion2D.
 */
class Tween2DAction : public Action
{
private:
    float* targetX;
    float* targetY;
    Motion2D motion;

public:
    Tween2DAction(float* x, float* y, float endX, float endY,
                  float durationSeconds,
                  Easing easing = Easing::Linear,
                  int loops = 1, bool swing = false);

    bool update(float dt) override;
};

/**
 * DelayAction - Waits for specified seconds.
 */
class DelayAction : public Action
{
private:
    float duration;
    float elapsed;

public:
    DelayAction(float seconds);

    bool update(float dt) override;
};

/**
 * CallAction - Executes a callback once, then completes.
 *
 * Useful for triggering events or side effects at specific points
 * in an action sequence.
 */
class CallAction : public Action
{
private:
    std::function<void()> callback;
    bool called;

public:
    CallAction(std::function<void()> fn);

    bool update(float dt) override;
};

/**
 * RepeatAction - Repeats an action N times.
 *
 * Uses a factory function to create fresh instances of the action
 * for each repetition.
 *
 * @param loops 0 = infinite, 1+ = that count
 */
class RepeatAction : public Action
{
private:
    std::function<std::unique_ptr<Action>()> factory;
    std::unique_ptr<Action> currentAction;
    int totalLoops;
    int currentLoop;

public:
    RepeatAction(std::function<std::unique_ptr<Action>()> actionFactory, int loops);

    void start() override;
    bool update(float dt) override;
    void stop() override;
};

/**
 * WaitUntilAction - Waits until condition returns true.
 *
 * Useful for waiting on user input or game state conditions.
 */
class WaitUntilAction : public Action
{
private:
    std::function<bool()> condition;

public:
    WaitUntilAction(std::function<bool()> condition);

    bool update(float dt) override;
};

/**
 * BlinkAction - Toggles a boolean visibility flag at regular intervals.
 *
 * Common operation for sprite blinking. Encapsulates the blink behavior.
 *
 * @param totalBlinks Number of toggles (0 = infinite, 6 = 3 full blinks)
 */
class BlinkAction : public Action
{
private:
    bool* visibleFlag;
    float interval;       // Seconds between toggles
    float elapsed;
    int totalBlinks;      // 0 = infinite
    int currentBlink;

public:
    BlinkAction(bool* visible, float intervalSeconds, int blinks);

    bool update(float dt) override;
};

//-----------------------------------------------------------------------------
// Helper Factory Functions
//-----------------------------------------------------------------------------

/**
 * Create a tween action for a float value
 */
inline std::unique_ptr<TweenAction> tweenTo(
    float* target, float end, float seconds,
    Easing e = Easing::Linear, int loops = 1, bool swing = false)
{
    return std::make_unique<TweenAction>(target, end, seconds, e, loops, swing);
}

/**
 * Create a tween action for an int value
 */
inline std::unique_ptr<TweenIntAction> tweenIntTo(
    int* target, int end, float seconds,
    Easing e = Easing::Linear, int loops = 1, bool swing = false)
{
    return std::make_unique<TweenIntAction>(target, end, seconds, e, loops, swing);
}

/**
 * Create a 2D tween action for x/y position
 */
inline std::unique_ptr<Tween2DAction> tween2D(
    float* x, float* y, float endX, float endY, float seconds,
    Easing e = Easing::Linear, int loops = 1, bool swing = false)
{
    return std::make_unique<Tween2DAction>(x, y, endX, endY, seconds, e, loops, swing);
}

/**
 * Create a delay action
 */
inline std::unique_ptr<DelayAction> delay(float seconds)
{
    return std::make_unique<DelayAction>(seconds);
}

/**
 * Create a callback action
 */
inline std::unique_ptr<CallAction> call(std::function<void()> fn)
{
    return std::make_unique<CallAction>(fn);
}

/**
 * Create a blink action
 */
inline std::unique_ptr<BlinkAction> blink(
    bool* visible, float interval, int times)
{
    return std::make_unique<BlinkAction>(visible, interval, times);
}
