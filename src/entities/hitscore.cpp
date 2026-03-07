#include "hitscore.h"
#include "../core/graph.h"
#include <cstdio>

HitScore::HitScore(BMFontRenderer* font, int cx, int cy, int score)
    : font(font), alpha(1.0f)
{
    xPos = (float)cx;
    yPos = (float)cy;
    snprintf(scoreText, sizeof(scoreText), "%d", score);

    // Rise upward with deceleration
    auto par = std::make_unique<ActionParallel>();
    par->add(tweenTo(&yPos, yPos - RISE_DISTANCE, TOTAL_DURATION, Easing::EaseOut));

    // Fade out after a short delay
    auto fadeSeq = std::make_unique<ActionSequence>();
    fadeSeq->then(delay(FADE_START))
           .then(tweenTo(&alpha, 0.0f, FADE_DURATION));
    par->add(std::move(fadeSeq));

    par->start();
    animation = std::move(par);
}

void HitScore::update(float dt)
{
    if (!animation->update(dt))
        kill();
}

void HitScore::draw(Graph* /*graph*/)
{
    unsigned char a = static_cast<unsigned char>(alpha * 255.0f);
    font->setColor(255, 80, 80, a);
    font->text(scoreText, (int)xPos, (int)yPos, TextAlign::Center);
    font->setColor(255, 255, 255, 255);
}
