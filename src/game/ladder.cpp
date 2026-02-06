#include "main.h"
#include "ladder.h"
#include "logger.h"

Ladder::Ladder(Scene* scene, int x, int y, int numTiles)
    : scene(scene), numTiles(numTiles)
{
    // Get ladder sprite from StageResources
    sprite = &gameinf.getStageRes().ladder;

    // Store tile dimensions from sprite
    tileWidth = sprite->getWidth();
    tileHeight = sprite->getHeight();
    totalHeight = tileHeight * numTiles;

    // Debug: log sprite dimensions to diagnose tile gap issue
    LOG_DEBUG("Ladder sprite: width=%d height=%d xoff=%d yoff=%d",
              sprite->getWidth(), sprite->getHeight(),
              sprite->getXOff(), sprite->getYOff());

    // Store position (bottom-middle anchor)
    // x = horizontal center, y = bottom of ladder
    this->xPos = (float)x;
    this->yPos = (float)y;
}

Ladder::~Ladder()
{
}

void Ladder::update(float dt)
{
    // Ladders are static - no update needed
}

void Ladder::draw(Graph& graph)
{
    // Convert from bottom-middle anchor to top-left for drawing
    // Top-left X = center X - half width
    int drawX = (int)xPos - tileWidth / 2;

    // Draw tiles from top to bottom
    // Top of ladder = yPos - totalHeight
    int topY = (int)yPos - totalHeight;

    for (int i = 0; i < numTiles; i++)
    {
        int tileY = topY + (i * tileHeight);
        graph.draw(sprite, drawX, tileY);
    }
}

CollisionBox Ladder::getCollisionBox() const
{
    // Full ladder collision box in top-left coordinates
    // Top-left X = center X - half width
    int boxX = (int)xPos - tileWidth / 2;
    int boxY = (int)yPos - totalHeight;

    return { boxX, boxY, tileWidth, totalHeight };
}

CollisionBox Ladder::getTopStandingBox() const
{
    // Thin box at the top of the ladder for standing detection
    // Height of 4 pixels should be enough for standing checks
    int boxX = (int)xPos - tileWidth / 2;
    int boxY = (int)yPos - totalHeight;

    return { boxX, boxY, tileWidth, 4 };
}
