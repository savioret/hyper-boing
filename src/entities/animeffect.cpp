#include "animeffect.h"
#include "../core/animspritesheet.h"
#include "../core/animcontroller.h"
#include "../core/graph.h"
#include "../core/sprite.h"
#include "../core/coordhelper.h"

AnimEffect::AnimEffect(int x, int y, AnimSpriteSheet* tmpl)
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

    int renderX = toRenderX(xPos, anim->getWidth());
	int renderY = toRenderY(yPos, anim->getHeight());
    graph->draw(spr, renderX, renderY);
}
