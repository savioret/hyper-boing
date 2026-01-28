/**
 * @file graph.h
 * @brief Graphics rendering system interface
 * 
 * This file contains the Graph class which provides graphics rendering
 * functionality using SDL2. It handles window creation, rendering operations,
 * sprite drawing, and bitmap loading.
 */

#pragma once

#include <SDL.h>
#include <string>

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
 * @brief Screen horizontal resolution in pixels
 */
constexpr int RES_X = 640;

/**
 * @brief Screen vertical resolution in pixels
 */
constexpr int RES_Y = 480;

class Graph;

/**
 * @brief Rendering properties for sprite drawing
 * 
 * Contains all properties needed to render a sprite, including position,
 * flipping, rotation, scale, and alpha blending.
 */
struct RenderProps
{
    int x, y;                    ///< Screen position
    bool flipH = false;          ///< Horizontal flip
    bool flipV = false;          ///< Vertical flip
    float rotation = 0.0f;       ///< Rotation angle in degrees
    float scale = 1.0f;          ///< Scale factor
    float alpha = 1.0f;          ///< Alpha transparency (0.0-1.0)
    float pivotX = 0.5f;         ///< Pivot X for rotation (0.0-1.0)
    float pivotY = 0.5f;         ///< Pivot Y for rotation (0.0-1.0)

    /**
     * @brief Construct from position
     */
    RenderProps(int x, int y) : x(x), y(y) {}

    /**
     * @brief Construct from Sprite2D properties
     */
    static RenderProps fromSprite2D(Sprite2D* spr);
};

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
     * @return 0 on success, non-zero on failure
     */
    int initNormal(const char* title);

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
};
