#include "animcontroller.h"

// ============================================================================
// FrameSequenceAnim Implementation
// ============================================================================

FrameSequenceAnim::FrameSequenceAnim(std::vector<int> frameSequence, int durationMs, bool shouldLoop)
    : frames(std::move(frameSequence))
    , defaultDuration(durationMs)
    , loop(shouldLoop)
    , usePerFrameDurations(false)
{
    if (frames.empty())
    {
        frames.push_back(0);  // Ensure at least one frame
    }
}

FrameSequenceAnim::FrameSequenceAnim(std::vector<int> frameSequence, std::vector<int> durationsMs, bool shouldLoop)
    : frames(std::move(frameSequence))
    , frameDurations(std::move(durationsMs))
    , defaultDuration(100)  // Fallback if frameDurations is empty
    , loop(shouldLoop)
    , usePerFrameDurations(true)
{
    if (frames.empty())
    {
        frames.push_back(0);  // Ensure at least one frame
    }
    
    // Ensure frameDurations matches frames size
    if (frameDurations.size() < frames.size())
    {
        frameDurations.resize(frames.size(), defaultDuration);
    }
}

FrameSequenceAnim FrameSequenceAnim::range(int startFrame, int endFrame, int durationMs, bool loop)
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
    return FrameSequenceAnim(std::move(seq), durationMs, loop);
}

FrameSequenceAnim FrameSequenceAnim::oscillate(int frameA, int frameB, int durationMs)
{
    return FrameSequenceAnim({frameA, frameB}, durationMs, true);
}

void FrameSequenceAnim::update(float dt)
{
    if (complete) return;

    timeAccumulator += dt;
    
    // Get current frame duration
    int currentDuration = usePerFrameDurations && currentIndex < static_cast<int>(frameDurations.size())
                          ? frameDurations[currentIndex]
                          : defaultDuration;
    
    if (timeAccumulator >= currentDuration)
    {
        timeAccumulator -= currentDuration;
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
    timeAccumulator = 0.0f;
    complete = false;
}

// ============================================================================
// ToggleAnim Implementation
// ============================================================================

ToggleAnim::ToggleAnim(int a, int b, int durationMs)
    : frameA(a)
    , frameB(b)
    , currentFrame(a)
    , toggleDuration(durationMs)
{
}

void ToggleAnim::update(float dt)
{
    timeAccumulator += dt;
    if (timeAccumulator >= toggleDuration)
    {
        timeAccumulator -= toggleDuration;
        currentFrame = (currentFrame == frameA) ? frameB : frameA;
    }
}

void ToggleAnim::reset()
{
    currentFrame = frameA;
    timeAccumulator = 0.0f;
}

// ============================================================================
// StateMachineAnim Implementation
// ============================================================================

void StateMachineAnim::addState(const std::string& name, std::vector<int> frames,
                                 int durationMs, bool loop, const std::string& nextState)
{
    State state;
    state.frames = std::move(frames);
    state.defaultDuration = durationMs;
    state.loop = loop;
    state.usePerFrameDurations = false;
    state.nextState = nextState;

    if (state.frames.empty())
    {
        state.frames.push_back(0);  // Ensure at least one frame
    }

    states[name] = std::move(state);
}

void StateMachineAnim::addState(const std::string& name, std::vector<int> frames,
                                 std::vector<int> durationsMs, bool loop, const std::string& nextState)
{
    State state;
    state.frames = std::move(frames);
    state.frameDurations = std::move(durationsMs);
    state.defaultDuration = 100;  // Fallback
    state.loop = loop;
    state.usePerFrameDurations = true;
    state.nextState = nextState;

    if (state.frames.empty())
    {
        state.frames.push_back(0);
    }
    
    // Ensure frameDurations matches frames size
    if (state.frameDurations.size() < state.frames.size())
    {
        state.frameDurations.resize(state.frames.size(), state.defaultDuration);
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
        timeAccumulator = 0.0f;
        stateComplete = false;
    }
}

void StateMachineAnim::update(float dt)
{
    if (currentStateName.empty()) return;

    auto it = states.find(currentStateName);
    if (it == states.end()) return;

    const State& state = it->second;

    timeAccumulator += dt;
    
    // Get current frame duration
    int currentDuration = state.usePerFrameDurations && currentIndex < static_cast<int>(state.frameDurations.size())
                          ? state.frameDurations[currentIndex]
                          : state.defaultDuration;
    
    if (timeAccumulator >= currentDuration)
    {
        timeAccumulator -= currentDuration;
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
    timeAccumulator = 0.0f;
    stateComplete = false;
    // Note: does not change current state, just resets its animation
}
