#pragma once

#include <SDL.h>
#include <vector>
#include <string>
#include "sprite.h"

// Forward declarations
class Graph;

/**
 * SpriteSheet - Manages multi-frame sprites from a single texture
 *
 * Handles sprite sheets where multiple animation frames are stored in a single
 * texture file. Each frame is a Sprite object sharing the same texture.
 *
 * Usage:
 *   SpriteSheet sheet;
 *   sheet.init(&graph, "texture.png");
 *   sheet.addFrame(0, 0, 16, 16, 0, 0);   // Frame 0
 *   sheet.addFrame(16, 0, 16, 16, 0, 0);  // Frame 1
 *
 *   // Render frame 0
 *   graph->draw(sheet.getFrame(0), x, y);
 */
class SpriteSheet
{
private:
    SDL_Texture* texture;              // Single texture shared by all frames
    std::vector<Sprite> frames;        // Frame sprites sharing the texture

public:
    SpriteSheet();
    ~SpriteSheet();

    /**
     * Initialize sprite sheet by loading texture from file
     * @param gr Graphics context
     * @param file Path to texture file
     * @return true if successful, false otherwise
     */
    bool init(Graph* gr, const std::string& file);

    /**
     * Add a frame definition to the sprite sheet
     * @param x X position of frame in texture
     * @param y Y position of frame in texture
     * @param w Width of frame
     * @param h Height of frame
     * @param xoff X offset for rendering (negative values move left)
     * @param yoff Y offset for rendering (negative values move up)
     */
    void addFrame(int x, int y, int w, int h, int xoff, int yoff);

    /**
     * Add a frame definition with original canvas size info
     * @param x X position of frame in texture
     * @param y Y position of frame in texture
     * @param w Width of frame (trimmed)
     * @param h Height of frame (trimmed)
     * @param xoff X offset for rendering (from Aseprite spriteSourceSize.x)
     * @param yoff Y offset for rendering (from Aseprite spriteSourceSize.y)
     * @param srcW Original canvas width (from Aseprite sourceSize.w)
     * @param srcH Original canvas height (from Aseprite sourceSize.h)
     */
    void addFrame(int x, int y, int w, int h, int xoff, int yoff, int srcW, int srcH);

    /**
     * Release texture resources
     */
    void release();

    /**
     * Get the underlying SDL texture
     */
    SDL_Texture* getTexture() const { return texture; }

    /**
     * Get a sprite for a specific frame
     * @param index Frame index (0-based)
     * @return Pointer to Sprite, or nullptr if index is invalid
     */
    Sprite* getFrame(int index);

    /**
     * Get total number of frames in the sprite sheet
     */
    int getFrameCount() const { return static_cast<int>(frames.size()); }
};
