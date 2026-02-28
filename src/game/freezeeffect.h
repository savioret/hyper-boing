#pragma once

#include <memory>
#include "action.h"
#include "bmfont.h"

class Graph;

/**
 * FreezeEffect
 *
 * Encapsulates the TIME_FREEZE pickup effect: countdown timer, ball-blink
 * via BlinkAction, and 3-2-1 countdown font rendering.
 *
 * Scene holds a single FreezeEffect member instead of five separate fields.
 */
class FreezeEffect
{
public:
    /**
     * Load the countdown font. Call once during scene initBitmaps().
     * @param graph Graph context used for rendering
     */
    void init(Graph* graph);

    /**
     * Begin a freeze of the given duration (seconds).
     * Resets any previous blink state.
     */
    void start(float duration);

    /**
     * End the freeze immediately (safe to call when already stopped).
     */
    void stop();

    /**
     * Advance the timer and drive the BlinkAction.
     * Calls stop() automatically when timer reaches zero.
     * @param dt Delta time in seconds
     */
    void update(float dt);

    /**
     * Render the 3-2-1 countdown centered in the game area.
     * No-op when not active or timer > BLINK_WARN.
     */
    void drawCountdown();

    bool  isActive()        const { return active; }
    float getTimer()        const { return timer; }
    bool  areBallsVisible() const { return ballsVisible; }

private:
    static constexpr float BLINK_WARN     = 3.0f;   ///< Seconds before end when balls start blinking
    static constexpr float BLINK_INTERVAL = 0.15f;  ///< Toggle interval in seconds

    bool  active       = false;
    float timer        = 0.0f;
    bool  ballsVisible = true;

    std::unique_ptr<BlinkAction>    blinkAction;
    std::unique_ptr<BMFontRenderer> countdownFont;
};
