#include "glass.h"
#include "main.h"

Glass::Glass(int x, int y, GlassType type)
    : type(type), damageLevel(0)
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

Sprite* Glass::getCurrentSprite() const
{
    int frame = static_cast<int>(type) + damageLevel;
    return gameinf.getStageRes().glassBricksAnim->getFrame(frame);
}
