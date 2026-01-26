#include "animcontroller.h"

// ============================================================================
// FrameSequenceAnim Implementation
// ============================================================================

FrameSequenceAnim::FrameSequenceAnim(std::vector<int> frameSequence, int delay, bool shouldLoop)
    : frames(std::move(frameSequence))
    , frameDelay(delay)
    , loop(shouldLoop)
{
    if (frames.empty())
    {
        frames.push_back(0);  // Ensure at least one frame
    }
}

FrameSequenceAnim FrameSequenceAnim::range(int startFrame, int endFrame, int delay, bool loop)
{
    std::vector<int> seq;
    if (startFrame <= endFrame)
    {
        for (int i = startFrame; i <= endFrame; ++i)
        {
            seq.push_back(i);
        }
    }
    else
    {
        for (int i = startFrame; i >= endFrame; --i)
        {
            seq.push_back(i);
        }
    }
    return FrameSequenceAnim(std::move(seq), delay, loop);
}

FrameSequenceAnim FrameSequenceAnim::oscillate(int frameA, int frameB, int delay)
{
    return FrameSequenceAnim({frameA, frameB}, delay, true);
}

void FrameSequenceAnim::update(float dt)
{
    if (complete) return;

    tickCounter++;
    if (tickCounter >= frameDelay)
    {
        tickCounter = 0;
        currentIndex++;

        if (currentIndex >= static_cast<int>(frames.size()))
        {
            if (loop)
            {
                currentIndex = 0;
            }
            else
            {
                currentIndex = static_cast<int>(frames.size()) - 1;
                complete = true;
                if (onComplete)
                {
                    onComplete();
                }
            }
        }
    }
}

int FrameSequenceAnim::getCurrentFrame() const
{
    if (currentIndex >= 0 && currentIndex < static_cast<int>(frames.size()))
    {
        return frames[currentIndex];
    }
    return frames.empty() ? 0 : frames[0];
}

void FrameSequenceAnim::reset()
{
    currentIndex = 0;
    tickCounter = 0;
    complete = false;
}

// ============================================================================
// ToggleAnim Implementation
// ============================================================================

ToggleAnim::ToggleAnim(int a, int b, int delay)
    : frameA(a)
    , frameB(b)
    , currentFrame(a)
    , toggleDelay(delay)
{
}

void ToggleAnim::update(float dt)
{
    tickCounter++;
    if (tickCounter >= toggleDelay)
    {
        tickCounter = 0;
        currentFrame = (currentFrame == frameA) ? frameB : frameA;
    }
}

void ToggleAnim::reset()
{
    currentFrame = frameA;
    tickCounter = 0;
}

// ============================================================================
// StateMachineAnim Implementation
// ============================================================================

void StateMachineAnim::addState(const std::string& name, std::vector<int> frames,
                                 int delay, bool loop, const std::string& nextState)
{
    State state;
    state.frames = std::move(frames);
    state.delay = delay;
    state.loop = loop;
    state.nextState = nextState;

    if (state.frames.empty())
    {
        state.frames.push_back(0);  // Ensure at least one frame
    }

    states[name] = std::move(state);
}

void StateMachineAnim::setState(const std::string& name)
{
    auto it = states.find(name);
    if (it != states.end())
    {
        currentStateName = name;
        currentIndex = 0;
        tickCounter = 0;
        stateComplete = false;
    }
}

void StateMachineAnim::update(float dt)
{
    if (currentStateName.empty()) return;

    auto it = states.find(currentStateName);
    if (it == states.end()) return;

    const State& state = it->second;

    tickCounter++;
    if (tickCounter >= state.delay)
    {
        tickCounter = 0;
        currentIndex++;

        if (currentIndex >= static_cast<int>(state.frames.size()))
        {
            if (state.loop)
            {
                currentIndex = 0;
            }
            else
            {
                currentIndex = static_cast<int>(state.frames.size()) - 1;
                stateComplete = true;

                // Callback before auto-transition
                if (onStateComplete)
                {
                    onStateComplete(currentStateName);
                }

                // Auto-transition to next state if specified
                if (!state.nextState.empty())
                {
                    setState(state.nextState);
                }
            }
        }
    }
}

int StateMachineAnim::getCurrentFrame() const
{
    if (currentStateName.empty()) return 0;

    auto it = states.find(currentStateName);
    if (it == states.end()) return 0;

    const State& state = it->second;
    if (currentIndex >= 0 && currentIndex < static_cast<int>(state.frames.size()))
    {
        return state.frames[currentIndex];
    }
    return state.frames.empty() ? 0 : state.frames[0];
}

void StateMachineAnim::reset()
{
    currentIndex = 0;
    tickCounter = 0;
    stateComplete = false;
    // Note: does not change current state, just resets its animation
}
