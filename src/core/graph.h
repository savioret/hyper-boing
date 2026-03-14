/**
 * @file graph.h
 * @brief Graphics rendering system interface
 */

#pragma once

#include <SDL.h>
#include <string>
#include "renderprops.h"

// Forward declarations
class Sprite;
class Sprite2D;
class BmNumFont;

/**
 * @brief Normal rendering mode
 * 
 * Standard rendering mode for regular gameplay.
 */
constexpr int RENDERMODE_NORMAL = 1;

/**
 * @brief Exclusive rendering mode
 * 
 * Exclusive fullscreen rendering mode.
 */
constexpr int RENDERMODE_EXCLUSIVE = 2;

/**
 * @brief Windowed 2x rendering mode
 * 
 * Windowed mode with 2x resolution (1280x960).
 */
constexpr int RENDERMODE_WINDOWED_2X = 3;

/**
 * @brief Screen horizontal resolution in pixels
 */
constexpr int RES_X = 640;

/**
 * @brief Screen vertical resolution in pixels
 */
constexpr int RES_Y = 480;

/**
 * @brief Text alignment constants for BmNumFont rendering
 */
constexpr int ALIGN_LEFT = 0;
constexpr int ALIGN_CENTER = 1;
constexpr int ALIGN_RIGHT = 2;

class Graph;

/**
 * @class Graph
 * @brief Graphics rendering and management system
 *
 * The Graph class provides a high-level interface for graphics operations
 * using SDL2. It handles:
 * - Graphics system initialization and cleanup
 * - Window and renderer management
 * - Sprite and texture rendering
 * - Primitive drawing operations (rectangles, text)
 * - Bitmap loading and color key management
 * - Screen flipping and fullscreen toggling
 * 
 * The class supports multiple rendering modes and provides both legacy
 * position-based drawing methods and modern methods that use internal
 * sprite properties.
 */
class Graph
{
private:
    SDL_Window* window;         ///< SDL window handle
    SDL_Renderer* renderer;     ///< SDL renderer handle
    SDL_Texture* backBuffer;    ///< Back buffer texture for double buffering
    int mode;                   ///< Current rendering mode (RENDERMODE_NORMAL or RENDERMODE_EXCLUSIVE)

public:
    /**
     * @brief Default constructor
     * 
     * Initializes all pointers to nullptr and mode to 0.
     */
    Graph() : window(nullptr), renderer(nullptr), backBuffer(nullptr), mode(0) {}

    /**
     * @brief Initialize the graphics system with specified mode
     * 
     * @param title Window title to display
     * @param mode Rendering mode (RENDERMODE_NORMAL or RENDERMODE_EXCLUSIVE)
     * @return 0 on success, non-zero on failure
     */
    int init(const char* title, int mode);

    /**
     * @brief Initialize graphics system in normal windowed mode
     * 
     * @param title Window title to display
     * @param windowWidth Window width in pixels
     * @param windowHeight Window height in pixels
     * @return 0 on success, non-zero on failure
     */
    int initNormal(const char* title, int windowWidth, int windowHeight);

    /**
     * @brief Initialize graphics system in exclusive mode
     * 
     * @param title Window title to display
     * @return 0 on success, non-zero on failure
     */
    int initEx(const char* title);

    /**
     * @brief Release all graphics resources
     * 
     * Cleans up and destroys the window, renderer, and back buffer.
     * Should be called before program termination.
     */
    void release();

    // Legacy draw methods (position passed explicitly)
    
    /**
     * @brief Draw a sprite at specified position
     * 
     * @param spr Pointer to the sprite to draw
     * @param x X coordinate on screen
     * @param y Y coordinate on screen
     */
    void draw(Sprite* spr, int x, int y);

    /**
     * @brief Draw a sprite at specified position with optional horizontal flip
     * 
     * @param spr Pointer to the sprite to draw
     * @param x X coordinate on screen
     * @param y Y coordinate on screen
     * @param flipHorizontal If true, flip the sprite horizontally
     */
    void draw(Sprite* spr, int x, int y, bool flipHorizontal);

    /**
     * @brief Draw a sprite with extended rendering properties
     *
     * @param spr Pointer to the sprite to draw
     * @param props Rendering properties (position, flip, rotation, scale, etc.)
     */
    void drawEx(Sprite* spr, const RenderProps& props);

    /**
     * @brief Draw a sprite with optional flash/tint effect
     *
     * Uses additive blending when flashWhite is true to create a solid
     * white silhouette effect (useful for hit feedback).
     *
     * @param spr Pointer to the sprite to draw
     * @param props Rendering properties (position, flip, rotation, scale, etc.)
     * @param flashWhite If true, render as solid white silhouette
     */
    void drawExFlash(Sprite* spr, const RenderProps& props, bool flashWhite);

    /**
     * @brief Draw a sprite scaled to specified dimensions
     *
     * @param spr Pointer to the sprite to draw
     * @param x X coordinate on screen
     * @param y Y coordinate on screen
     * @param w Target width in pixels
     * @param h Target height in pixels
     */
    void drawScaled(Sprite* spr, int x, int y, int w, int h);

    /**
     * @brief Draw a sprite with vertical clipping (top portion only)
     *
     * Draws only the top portion of the sprite, useful for chain tiles
     * that shouldn't extend past a boundary.
     *
     * @param spr Pointer to the sprite to draw
     * @param x X coordinate on screen
     * @param y Y coordinate on screen
     * @param visibleHeight Number of pixels to draw from top of sprite
     */
    void drawClipped(Sprite* spr, int x, int y, int visibleHeight);

    /**
     * @brief Draw a texture or portion of a texture
     * 
     * @param texture SDL texture to draw
     * @param srcRect Source rectangle (nullptr for entire texture)
     * @param x X coordinate on screen
     * @param y Y coordinate on screen
     */
    void draw(SDL_Texture* texture, const SDL_Rect* srcRect, int x, int y);

    // New draw methods (uses Sprite2D's internal properties)
    
    /**
     * @brief Draw a Sprite2D using its internal position properties
     * 
     * @param spr Pointer to the Sprite2D to draw
     */
    void draw(Sprite2D* spr);

    /**
     * @brief Draw a number using bitmap font
     *
     * @param font Pointer to the bitmap font
     * @param num Number to draw
     * @param x X coordinate on screen
     * @param y Y coordinate on screen
     */
    void draw(BmNumFont* font, int num, int x, int y);

    /**
     * @brief Draw a number using bitmap font with alignment
     *
     * @param font Pointer to the bitmap font
     * @param num Number to draw
     * @param x X coordinate on screen (interpreted based on alignment)
     * @param y Y coordinate on screen
     * @param align Text alignment (Left, Center, Right)
     */
    void draw(BmNumFont* font, int num, int x, int y, int align);

    /**
     * @brief Draw a string using bitmap font
     *
     * @param font Pointer to the bitmap font
     * @param cad String to draw
     * @param x X coordinate on screen
     * @param y Y coordinate on screen
     */
    void draw(BmNumFont* font, const std::string& cad, int x, int y);

    /**
     * @brief Flip the back buffer to screen
     * 
     * Presents the rendered frame to the display. Should be called
     * once per frame after all drawing operations are complete.
     */
    void flip();

    /**
     * @brief Toggle fullscreen mode
     * 
     * @param fs If true, switch to fullscreen; if false, switch to windowed mode
     */
    void setFullScreen(bool fs);

    /**
     * @brief Set the window size
     * 
     * @param windowWidth Window width in pixels
     * @param windowHeight Window height in pixels
     */
    void setWindowSize(int windowWidth, int windowHeight);

    /**
     * @brief Set the render mode
     * 
     * @param newMode The new render mode to set
     */
    void setMode(int newMode) { mode = newMode; }

    /**
     * @brief Draw text on screen
     * 
     * Uses SDL's built-in text rendering capabilities.
     * 
     * @param texto Text string to draw
     * @param x X coordinate on screen
     * @param y Y coordinate on screen
     */
    void text(const char texto[], int x, int y);

    /**
     * @brief Set the drawing color for subsequent primitive operations
     * 
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     * @param a Alpha component (0-255), defaults to 255 (opaque)
     */
    void setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);

    /**
     * @brief Draw an unfilled rectangle
     * 
     * @param a Left X coordinate
     * @param b Top Y coordinate
     * @param c Right X coordinate
     * @param d Bottom Y coordinate
     */
    void rectangle(int a, int b, int c, int d);

    /**
     * @brief Draw a filled rectangle
     *
     * @param a Left X coordinate
     * @param b Top Y coordinate
     * @param c Right X coordinate
     * @param d Bottom Y coordinate
     */
    void filledRectangle(int a, int b, int c, int d);

    /**
     * @brief Draw an arrow from (x0,y0) to (x1,y1)
     *
     * Renders a thick line with an arrowhead at the tip.
     *
     * @param x0        Start X
     * @param y0        Start Y
     * @param x1        End X (tip)
     * @param y1        End Y (tip)
     * @param thickness Line thickness in pixels (default 2)
     */
    void drawArrow(int x0, int y0, int x1, int y1, int thickness = 2);

    /**
     * @brief Draw an unfilled circle
     *
     * @param cx Center X coordinate
     * @param cy Center Y coordinate
     * @param radius Radius in pixels
     */
    void circle(int cx, int cy, int radius);

    /**
     * @brief Copy a bitmap surface to a texture
     * 
     * @param texture Reference to texture pointer (output)
     * @param surface SDL surface containing the bitmap data
     * @param x X offset in the source surface
     * @param y Y offset in the source surface
     * @param dx Width to copy
     * @param dy Height to copy
     * @return true on success, false on failure
     */
    bool copyBitmap(SDL_Texture*& texture, SDL_Surface* surface, int x, int y, int dx, int dy);

    /**
     * @brief Load a bitmap file into a sprite
     * 
     * @param spr Pointer to the sprite to load into
     * @param szBitmap Path to the bitmap file
     */
    void loadBitmap(Sprite* spr, const char* szBitmap);

    /**
     * @brief Match a color in a surface's pixel format
     * 
     * @param surface SDL surface with the target pixel format
     * @param rgb RGB color value to match
     * @return Converted color value in the surface's format
     */
    Uint32 colorMatch(SDL_Surface* surface, Uint32 rgb);

    /**
     * @brief Set the transparent color key for a texture
     * 
     * @param texture SDL texture to modify
     * @param rgb RGB color value to use as transparent
     * @return true on success, false on failure
     */
    bool setColorKey(SDL_Texture* texture, Uint32 rgb);

    /**
     * @brief Get the SDL renderer
     * 
     * @return Pointer to the SDL_Renderer instance
     */
    SDL_Renderer* getRenderer() const { return renderer; }
    SDL_Window*   getWindow()   const { return window; }
};
