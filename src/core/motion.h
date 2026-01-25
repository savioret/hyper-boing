#pragma once

#include <functional>

/**
 * Easing types for motion interpolation
 */
enum class Easing
{
    Linear,     // Constant speed
    EaseIn,     // Slow start, fast end
    EaseOut,    // Fast start, slow end
    EaseInOut   // Slow start and end, fast middle
};

/**
 * Easing function type: takes progress [0,1] and returns eased value [0,1]
 */
using EasingFunction = std::function<float(float)>;

/**
 * Get easing function by enum
 */
EasingFunction getEasingFunction(Easing easing);

/**
 * Motion - Interpolates a single float value over time.
 *
 * Uses seconds-based duration with delta time accumulation.
 * Supports looping and swing (ping-pong) modes.
 *
 * Usage:
 *   Motion motion(0.0f, 100.0f, 2.0f, Easing::EaseOut);
 *   while (motion.update(deltaTime)) {
 *       float currentValue = motion.value();
 *   }
 */
class Motion
{
private:
    float startValue;
    float endValue;
    float duration;        // Seconds
    float elapsed;
    Easing easing;

    int loops;             // 0 = infinite, 1 = once, 2+ = that many times
    int currentLoop;
    bool swing;            // If true, alternates direction each loop
    bool forward;          // Current direction (for swing)

public:
    /**
     * Constructor
     * @param start Starting value
     * @param end Target value
     * @param durationSeconds Animation duration in seconds
     * @param easing Easing function to use
     * @param loops Number of loops (0=infinite, 1=once, 2+=that many)
     * @param swing If true, ping-pongs between start and end
     */
    Motion(float start, float end, float durationSeconds,
           Easing easing = Easing::Linear,
           int loops = 1, bool swing = false);

    /**
     * Update with delta time
     * @param dt Delta time in seconds
     * @return true while running, false when all loops complete
     */
    bool update(float dt);

    /**
     * Get current interpolated value
     */
    float value() const;

    /**
     * Check if all loops are complete
     */
    bool finished() const;

    /**
     * Reset to beginning
     */
    void reset();

    /**
     * Get progress [0, 1] within current loop
     */
    float progress() const;
};

/**
 * Motion2D - Interpolates x and y values together.
 *
 * Useful for sprite/object position animations.
 * Both axes share the same duration, easing, loops, and swing settings.
 *
 * Usage:
 *   Motion2D motion(0.0f, 0.0f, 100.0f, 200.0f, 1.0f);
 *   while (motion.update(deltaTime)) {
 *       float x = motion.x();
 *       float y = motion.y();
 *   }
 */
class Motion2D
{
private:
    Motion motionX;
    Motion motionY;

public:
    /**
     * Constructor
     * @param startX Starting X value
     * @param startY Starting Y value
     * @param endX Target X value
     * @param endY Target Y value
     * @param durationSeconds Animation duration in seconds
     * @param easing Easing function to use
     * @param loops Number of loops (0=infinite, 1=once, 2+=that many)
     * @param swing If true, ping-pongs between start and end
     */
    Motion2D(float startX, float startY, float endX, float endY,
             float durationSeconds, Easing easing = Easing::Linear,
             int loops = 1, bool swing = false);

    /**
     * Update both axes with delta time
     * @param dt Delta time in seconds
     * @return true while running, false when both axes complete
     */
    bool update(float dt);

    /**
     * Get current X value
     */
    float x() const { return motionX.value(); }

    /**
     * Get current Y value
     */
    float y() const { return motionY.value(); }

    /**
     * Check if both axes are complete
     */
    bool finished() const { return motionX.finished() && motionY.finished(); }

    /**
     * Reset both axes to beginning
     */
    void reset();
};
