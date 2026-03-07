#pragma once

#include "../core/gameobject.h"
#include "../core/action.h"
#include "../ui/bmfont.h"
#include <memory>

class Graph;

/**
 * HitScore - Floating score number that appears when a ball is popped.
 *
 * Spawned at (cx, cy); rises 50 px upward with EaseOut deceleration over 1.5 s.
 * Fades out starting at 700 ms, completing at 1500 ms.
 *
 * Inherits from IGameObject for automatic lifecycle management in Scene.
 * The owner (Scene) must keep the BMFontRenderer alive for the object's lifetime.
 */
class HitScore : public IGameObject
{
public:
    HitScore(BMFontRenderer* font, int cx, int cy, int score);

    void update(float dt);
    void draw(Graph* graph);

    CollisionBox getCollisionBox() const override { return {0, 0, 0, 0}; }

private:
    static constexpr float RISE_DISTANCE = 30.0f;  ///< Pixels to rise upward
    static constexpr float TOTAL_DURATION = 1.5f;  ///< Total animation duration (s)
    static constexpr float FADE_START = 0.7f;       ///< Delay before fade begins (s)
    static constexpr float FADE_DURATION = 0.8f;    ///< Fade-out duration (s)

    BMFontRenderer* font;           ///< Non-owning; Scene manages lifetime
    float alpha;                    ///< Current alpha [0..1], tweened to 0
    std::unique_ptr<Action> animation;
    char scoreText[16];
};
