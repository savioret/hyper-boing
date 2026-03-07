#include "glass.h"
#include "main.h"
#include "scene.h"

Glass::Glass(Scene* scn, int x, int y, GlassType type)
    : type(type), damageLevel(0), scene(scn)
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
    ++damageLevel;
    if (damageLevel > 4)
        kill();
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
    int frame = static_cast<int>(type) + damageLevel;
    return gameinf.getStageRes().glassBricksAnim->getFrame(frame);
}
