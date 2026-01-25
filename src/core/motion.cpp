#include "motion.h"
#include "logger.h"
#include <cmath>
#include <algorithm>

// Easing function implementations
namespace {
    float easeLinear(float t) {
        return t;
    }

    float easeIn(float t) {
        return t * t;  // Quadratic ease in
    }

    float easeOut(float t) {
        return t * (2.0f - t);  // Quadratic ease out
    }

    float easeInOut(float t) {
        if (t < 0.5f)
            return 2.0f * t * t;
        else
            return -1.0f + (4.0f - 2.0f * t) * t;
    }
}

EasingFunction getEasingFunction(Easing easing)
{
    switch (easing)
    {
        case Easing::Linear:   return easeLinear;
        case Easing::EaseIn:   return easeIn;
        case Easing::EaseOut:  return easeOut;
        case Easing::EaseInOut: return easeInOut;
        default:               return easeLinear;
    }
}

// Motion implementation

Motion::Motion(float start, float end, float durationSeconds,
               Easing easing, int loops, bool swing)
    : startValue(start), endValue(end), duration(durationSeconds),
      elapsed(0.0f), easing(easing), loops(loops), currentLoop(0),
      swing(swing), forward(true)
{
}

bool Motion::update(float dt)
{
    if (finished()) return false;

    elapsed += dt;

    // Check if current loop is complete
    if (elapsed >= duration)
    {
        // Handle loop completion
        if (loops == 0)  // Infinite loops
        {
            elapsed -= duration;
            if (swing)
                forward = !forward;
        }
        else if (currentLoop + 1 < loops)  // More loops to go
        {
            currentLoop++;
            elapsed -= duration;
            if (swing)
                forward = !forward;
        }
        else  // All loops done
        {
            elapsed = duration;
            return false;
        }
    }

    return true;
}

float Motion::value() const
{
    if (duration <= 0.0f) return endValue;

    // Calculate raw progress [0, 1]
    float t = std::min(1.0f, elapsed / duration);

    // Apply easing
    EasingFunction easingFunc = getEasingFunction(easing);
    float easedT = easingFunc(t);

    // Calculate interpolated value
    float currentStart = forward ? startValue : endValue;
    float currentEnd = forward ? endValue : startValue;

    return currentStart + (currentEnd - currentStart) * easedT;
}

bool Motion::finished() const
{
    if (loops == 0) return false;  // Infinite never finishes
    return currentLoop >= loops - 1 && elapsed >= duration;
}

void Motion::reset()
{
    elapsed = 0.0f;
    currentLoop = 0;
    forward = true;
}

float Motion::progress() const
{
    if (duration <= 0.0f) return 1.0f;
    return std::min(1.0f, elapsed / duration);
}

// Motion2D implementation

Motion2D::Motion2D(float startX, float startY, float endX, float endY,
                   float durationSeconds, Easing easing, int loops, bool swing)
    : motionX(startX, endX, durationSeconds, easing, loops, swing),
      motionY(startY, endY, durationSeconds, easing, loops, swing)
{
}

bool Motion2D::update(float dt)
{
    bool xRunning = motionX.update(dt);
    bool yRunning = motionY.update(dt);
    return xRunning || yRunning;
}

void Motion2D::reset()
{
    motionX.reset();
    motionY.reset();
}
