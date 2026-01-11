#pragma once

#include <SDL.h>
#include <string>

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
class BmNumFont;

/**
 * Sprite class
 *
 * This is the Sprite object, as its name suggests.
 * It contains information about the bitmap, its dimensions to manage
 * collisions, and the "relative" displacement.
 *
 * xoff and yoff are used to represent animation sequences.
 * For example, if a character is walking, all its frames must be aligned
 * with its body, otherwise it would look like it's doing the "dance of Saint Vitus".
 * Xoff and Yoff help to adjust and correct this.
 */
class Sprite
{
private:
    SDL_Texture* bmp;
    int sx, sy;
    int xoff, yoff; // relative displacement
    Graph* graph;

public:
    Sprite() : bmp(nullptr), sx(0), sy(0), xoff(0), yoff(0), graph(nullptr) {}

    void init(Graph* gr, const std::string& file, int offx = 0, int offy = 0);
    void release();

    SDL_Texture* getBmp() const { return bmp; }
    int getWidth() const { return sx; }
    int getHeight() const { return sy; }
    int getXOff() const { return xoff; }
    int getYOff() const { return yoff; }

    void setBmp(SDL_Texture* texture) { bmp = texture; }
    void setWidth(int w) { sx = w; }
    void setHeight(int h) { sy = h; }

    // Temporary friend class or public access until refactoring is complete
    friend class Graph;
    friend class Floor;
    friend class Scene;
};

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
