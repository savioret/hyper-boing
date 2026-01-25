#include "action.h"

// Action base class

void Action::finish()
{
    if (!isDone)
    {
        isDone = true;
        stop();
    }
}

// ActionSequence

ActionSequence::ActionSequence()
    : currentIndex(0), started(false)
{
}

ActionSequence& ActionSequence::then(std::unique_ptr<Action> action)
{
    actions.push_back(std::move(action));
    return *this;
}

void ActionSequence::start()
{
    started = true;
    if (!actions.empty() && currentIndex < actions.size())
    {
        actions[currentIndex]->start();
    }
}

bool ActionSequence::update(float dt)
{
    if (isDone) return false;

    // Auto-start if not started
    if (!started)
    {
        start();
        if (actions.empty())
        {
            isDone = true;
            return false;
        }
    }

    // Update current action
    if (currentIndex < actions.size())
    {
        if (actions[currentIndex]->update(dt))
        {
            return true;  // Current action still running
        }

        // Current action finished, move to next
        currentIndex++;

        if (currentIndex < actions.size())
        {
            actions[currentIndex]->start();
            return true;  // Continue with next action
        }
    }

    // All actions complete
    isDone = true;
    return false;
}

void ActionSequence::stop()
{
    // Stop current action if any
    if (currentIndex < actions.size())
    {
        actions[currentIndex]->stop();
    }
}

// ActionParallel

ActionParallel::ActionParallel()
    : started(false)
{
}

ActionParallel& ActionParallel::add(std::unique_ptr<Action> action)
{
    actions.push_back(std::move(action));
    return *this;
}

void ActionParallel::start()
{
    started = true;
    for (auto& action : actions)
    {
        action->start();
    }
}

bool ActionParallel::update(float dt)
{
    if (isDone) return false;

    // Auto-start if not started
    if (!started)
    {
        start();
        if (actions.empty())
        {
            isDone = true;
            return false;
        }
    }

    // Update all actions
    bool anyRunning = false;
    for (auto& action : actions)
    {
        if (!action->isFinished())
        {
            if (action->update(dt))
            {
                anyRunning = true;
            }
        }
    }

    // Complete when all actions are done
    if (!anyRunning)
    {
        isDone = true;
        return false;
    }

    return true;
}

void ActionParallel::stop()
{
    for (auto& action : actions)
    {
        if (!action->isFinished())
        {
            action->stop();
        }
    }
}

// TweenAction

TweenAction::TweenAction(float* target, float endValue, float durationSeconds,
                         Easing easing, int loops, bool swing)
    : target(target),
      motion(target ? *target : 0.0f, endValue, durationSeconds, easing, loops, swing)
{
}

bool TweenAction::update(float dt)
{
    if (isDone) return false;

    bool running = motion.update(dt);

    if (target)
    {
        *target = motion.value();
    }

    if (!running)
    {
        isDone = true;
    }

    return running;
}

// TweenIntAction

TweenIntAction::TweenIntAction(int* target, int endValue, float durationSeconds,
                               Easing easing, int loops, bool swing)
    : target(target),
      motion(target ? static_cast<float>(*target) : 0.0f,
             static_cast<float>(endValue), durationSeconds, easing, loops, swing)
{
}

bool TweenIntAction::update(float dt)
{
    if (isDone) return false;

    bool running = motion.update(dt);

    if (target)
    {
        *target = static_cast<int>(motion.value());
    }

    if (!running)
    {
        isDone = true;
    }

    return running;
}

// Tween2DAction

Tween2DAction::Tween2DAction(float* x, float* y, float endX, float endY,
                             float durationSeconds,
                             Easing easing, int loops, bool swing)
    : targetX(x), targetY(y),
      motion(x ? *x : 0.0f, y ? *y : 0.0f, endX, endY,
             durationSeconds, easing, loops, swing)
{
}

bool Tween2DAction::update(float dt)
{
    if (isDone) return false;

    bool running = motion.update(dt);

    if (targetX) *targetX = motion.x();
    if (targetY) *targetY = motion.y();

    if (!running)
    {
        isDone = true;
    }

    return running;
}

// DelayAction

DelayAction::DelayAction(float seconds)
    : duration(seconds), elapsed(0.0f)
{
}

bool DelayAction::update(float dt)
{
    if (isDone) return false;

    elapsed += dt;

    if (elapsed >= duration)
    {
        isDone = true;
        return false;
    }

    return true;
}

// CallAction

CallAction::CallAction(std::function<void()> fn)
    : callback(fn), called(false)
{
}

bool CallAction::update(float dt)
{
    if (isDone) return false;

    if (!called)
    {
        if (callback)
        {
            callback();
        }
        called = true;
    }

    isDone = true;
    return false;
}

// RepeatAction

RepeatAction::RepeatAction(std::function<std::unique_ptr<Action>()> actionFactory, int loops)
    : factory(actionFactory), totalLoops(loops), currentLoop(0)
{
}

void RepeatAction::start()
{
    if (factory)
    {
        currentAction = factory();
        if (currentAction)
        {
            currentAction->start();
        }
    }
}

bool RepeatAction::update(float dt)
{
    if (isDone) return false;

    // Check if we have an action to run
    if (!currentAction)
    {
        isDone = true;
        return false;
    }

    // Update current action
    if (currentAction->update(dt))
    {
        return true;  // Still running
    }

    // Current action finished
    currentLoop++;

    // Check if we should repeat
    if (totalLoops == 0 || currentLoop < totalLoops)
    {
        // Create and start next iteration
        currentAction = factory();
        if (currentAction)
        {
            currentAction->start();
            return true;
        }
        else
        {
            isDone = true;
            return false;
        }
    }

    // All loops complete
    isDone = true;
    return false;
}

void RepeatAction::stop()
{
    if (currentAction && !currentAction->isFinished())
    {
        currentAction->stop();
    }
}

// WaitUntilAction

WaitUntilAction::WaitUntilAction(std::function<bool()> condition)
    : condition(condition)
{
}

bool WaitUntilAction::update(float dt)
{
    if (isDone) return false;

    if (condition && condition())
    {
        isDone = true;
        return false;
    }

    return true;
}

// BlinkAction

BlinkAction::BlinkAction(bool* visible, float intervalSeconds, int blinks)
    : visibleFlag(visible), interval(intervalSeconds),
      elapsed(0.0f), totalBlinks(blinks), currentBlink(0)
{
}

bool BlinkAction::update(float dt)
{
    if (isDone) return false;

    elapsed += dt;

    if (elapsed >= interval)
    {
        elapsed -= interval;

        // Toggle visibility
        if (visibleFlag)
        {
            *visibleFlag = !(*visibleFlag);
        }

        currentBlink++;

        // Check if we're done
        if (totalBlinks > 0 && currentBlink >= totalBlinks)
        {
            isDone = true;
            return false;
        }
    }

    return true;
}
