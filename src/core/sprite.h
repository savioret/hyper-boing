#pragma once

#include <SDL.h>
#include <string>

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
    int sx, sy;          // Width and height
    int srcX, srcY;      // Source position in texture (for sprite sheets)
    int xoff, yoff;      // Relative displacement
    Graph* graph;

public:
    Sprite() : bmp(nullptr), sx(0), sy(0), srcX(0), srcY(0), xoff(0), yoff(0), graph(nullptr) {}

    void init(Graph* gr, const std::string& file, int offx = 0, int offy = 0);
    void init(SDL_Texture* sharedTexture, int x, int y, int w, int h, int offx, int offy);
    void release();

    SDL_Texture* getBmp() const { return bmp; }
    int getWidth() const { return sx; }
    int getHeight() const { return sy; }
    int getSrcX() const { return srcX; }
    int getSrcY() const { return srcY; }
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
