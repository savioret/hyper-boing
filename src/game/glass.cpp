#include "glass.h"
#include "main.h"
#include "scene.h"

Glass::Glass(Scene* scn, int x, int y, GlassType type)
    : type(type), scene(scn)
{
    xPos = (float)x;
    yPos = (float)y;

    // Get collision-box dimensions from the first (undamaged) frame
    int firstFrame = static_cast<int>(type);
    Sprite* spr = gameinf.getStageRes().glassBricksAnim->getFrame(firstFrame);
    if (spr)
    {
        sx = spr->getWidth();
        sy = spr->getHeight();
    }
    else
    {
        sx = 16;
        sy = 16;
    }
}

void Glass::onHit()
{
    if (breaking)
        return;  // Already breaking, ignore further hits

    breaking = true;

    // Create destruction animation: 5 frames from type to type+4, play once
    int startFrame = static_cast<int>(type);
    int endFrame = startFrame + 4;
    breakAnim = std::make_unique<FrameSequenceAnim>(
        FrameSequenceAnim::range(startFrame, endFrame, 80, 1));  // 80ms per frame, play once
}

void Glass::update(float dt)
{
    if (breaking && breakAnim)
    {
        // Convert dt from seconds to milliseconds for animation controller
        float dtMs = dt * 1000.0f;
        breakAnim->update(dtMs);
        if (breakAnim->isComplete())
            kill();
    }
}

void Glass::onDeath()
{
    if (hasDeathPickup && scene)
    {
        int cx = (int)xPos + sx / 2;
        int cy = (int)yPos + sy / 2;
        scene->addPickup(cx, cy, deathPickupType);
    }
}

Sprite* Glass::getCurrentSprite() const
{
    int frame = static_cast<int>(type);
    if (breaking && breakAnim)
        frame = breakAnim->getCurrentFrame();
    return gameinf.getStageRes().glassBricksAnim->getFrame(frame);
}
