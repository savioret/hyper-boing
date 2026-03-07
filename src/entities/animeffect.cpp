#include "animeffect.h"
#include "../core/animspritesheet.h"
#include "../core/animcontroller.h"
#include "../core/graph.h"
#include "../core/sprite.h"
#include "../core/coordhelper.h"

AnimEffect::AnimEffect(int x, int y, AnimSpriteSheet* tmpl, float scaleVal)
    : scale(scaleVal)
{
    xPos = (float)x;
    yPos = (float)y;
    anim = tmpl->clone();

    // Effects always play once - force the underlying animation to stop after one pass.
    // FrameSequenceAnim exposes setLoops(); StateMachineAnim states are configured per-state.
    if (auto* seq = dynamic_cast<FrameSequenceAnim*>(anim->getAnimController()))
        seq->setLoops(1);
}

void AnimEffect::update(float dt)
{
    float dtMs = dt * 1000.0f;
    anim->update(dtMs);

    if (anim->isComplete())
        kill();
}

void AnimEffect::draw(Graph* graph)
{
    Sprite* spr = anim->getCurrentSprite();
    if (!spr)
        return;

    // Apply scale to get render position (centered)
    int scaledW = (int)(anim->getWidth() * scale);
    int scaledH = (int)(anim->getHeight() * scale);
    int renderX = toRenderX(xPos, scaledW);
    int renderY = toRenderY(yPos, scaledH);

    if (scale != 1.0f)
    {
        // Use extended draw with scale
        RenderProps props;
        props.x = renderX;
        props.y = renderY;
        props.scale = scale;
        props.pivotX = 0.0f;  // Top-left pivot since we already calculated render position
        props.pivotY = 0.0f;
        graph->drawEx(spr, props);
    }
    else
    {
        graph->draw(spr, renderX, renderY);
    }
}
