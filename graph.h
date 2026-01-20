#pragma once

#include <SDL.h>
#include <string>

// Forward declarations
class Sprite;
class BmNumFont;

/**
 * Render modes
 */
constexpr int RENDERMODE_NORMAL = 1;
constexpr int RENDERMODE_EXCLUSIVE = 2;

/**
 * Screen resolution
 */
constexpr int RES_X = 640;
constexpr int RES_Y = 480;

class Graph;

/**
 * Graph class
 *
 * This is the class that performs "primitive" painting functions.
 * Similarly, it initializes the graphic mode and restores it.
 * It also loads BMP images.
 */
class Graph
{
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* backBuffer;
    int mode;

public:
    Graph() : window(nullptr), renderer(nullptr), backBuffer(nullptr), mode(0) {}

    int init(const char* title, int mode);
    int initNormal(const char* title);
    int initEx(const char* title);
    void release();

    void draw(Sprite* spr, int x, int y);
    void drawScaled(Sprite* spr, int x, int y, int w, int h);
    void draw(SDL_Texture* texture, const SDL_Rect* srcRect, int x, int y);
    void drawClipped(SDL_Texture* texture, const SDL_Rect* srcRect, int x, int y);
    void drawClipped(Sprite* spr, int x, int y);
    void draw(BmNumFont* font, int num, int x, int y);
    void draw(BmNumFont* font, const std::string& cad, int x, int y);
    void drawClipped(BmNumFont* font, const std::string& cad, int x, int y);

    void flip();
    void setFullScreen(bool fs);

    void text(const char texto[], int x, int y);
    void rectangle(int a, int b, int c, int d);
    void filledRectangle(int a, int b, int c, int d);

    bool copyBitmap(SDL_Texture*& texture, SDL_Surface* surface, int x, int y, int dx, int dy);
    void loadBitmap(Sprite* spr, const char* szBitmap);
    Uint32 colorMatch(SDL_Surface* surface, Uint32 rgb);
    bool setColorKey(SDL_Texture* texture, Uint32 rgb);

    SDL_Renderer* getRenderer() const { return renderer; }
};
