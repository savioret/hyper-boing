#include "freezeeffect.h"
#include "stage.h"
#include "graph.h"
#include "logger.h"
#include <cstdio>

void FreezeEffect::init(Graph* graph)
{
    countdownFont = std::make_unique<BMFontRenderer>();
    if (!countdownFont->loadFont(graph, "assets/fonts/thickfont_grad_32.fnt"))
    {
        LOG_WARNING("Failed to load freeze countdown font (thickfont_grad_32.fnt)");
        countdownFont.reset();
    }
}

void FreezeEffect::start(float duration)
{
    active = true;
    timer = duration;
    ballsVisible = true;
    blinkAction.reset();
    LOG_DEBUG("Time frozen for %.1f seconds", duration);
}

void FreezeEffect::stop()
{
    active = false;
    timer = 0.0f;
    ballsVisible = true;
    blinkAction.reset();
    LOG_DEBUG("Time unfrozen");
}

void FreezeEffect::update(float dt)
{
    if (!active) return;

    timer -= dt;
    if (timer <= 0.0f)
    {
        stop();
        return;
    }

    if (timer <= BLINK_WARN)
    {
        // Lazily create the BlinkAction the first time we enter the warning phase
        if (!blinkAction)
        {
            blinkAction = blink(&ballsVisible, BLINK_INTERVAL, 0);
        }
        blinkAction->update(dt);
    }
}

void FreezeEffect::drawCountdown()
{
    if (!active || timer > BLINK_WARN || !countdownFont) return;

    int countdownVal = (int)timer + 1;  // 3, 2, 1
    char buf[4];
    std::snprintf(buf, sizeof(buf), "%d", countdownVal);

    int textW = countdownFont->getTextWidth(buf);
    int textH = countdownFont->getTextHeight();
    int x = (Stage::MIN_X + Stage::MAX_X) / 2 - textW / 2;
    int y = (int)(Stage::MAX_Y * 0.2f) - textH / 2;
    countdownFont->text(buf, x, y);
}
