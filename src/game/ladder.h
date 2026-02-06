#pragma once

#include "gameobject.h"

class Scene;
class Sprite;
class Graph;

/**
 * Ladder class
 *
 * A climbable vertical structure composed of tiled sprites.
 * Players can climb up/down when positioned near the ladder.
 *
 * Uses BOTTOM-MIDDLE anchor (like Player):
 * - xPos = horizontal center of ladder
 * - yPos = bottom of ladder (ground level)
 * - Top of ladder = yPos - totalHeight
 */
class Ladder : public IGameObject
{
private:
    Scene* scene;
    Sprite* sprite;           // Single tile sprite (e.g., 22x16)
    int tileWidth;
    int tileHeight;
    int numTiles;             // Number of vertical tiles
    int totalHeight;          // tileHeight * numTiles

public:
    Ladder(Scene* scene, int x, int y, int numTiles);
    ~Ladder();

    void update(float dt);
    void draw(Graph& graph);

    // Position helpers (bottom-middle anchor)
    int getTopY() const { return (int)yPos - totalHeight; }
    int getBottomY() const { return (int)yPos; }
    int getCenterX() const { return (int)xPos; }
    int getTileWidth() const { return tileWidth; }
    int getTileHeight() const { return tileHeight; }
    int getNumTiles() const { return numTiles; }
    int getTotalHeight() const { return totalHeight; }

    /**
     * Get collision box for full ladder extent
     * Returns box in top-left coordinate space for collision detection
     */
    CollisionBox getCollisionBox() const override;

    /**
     * Get box for standing on top of ladder
     * A thin horizontal box at the top of the ladder
     */
    CollisionBox getTopStandingBox() const;
};
