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

    // Animatable properties
    float x, y;          // Position (absolute world coordinates)
    float alpha;         // Alpha channel (0-255)
    float scale;         // Uniform scale (1.0 = normal size)
    float rotation;      // Rotation angle in degrees (clockwise)
    float pivotX, pivotY; // Pivot point (0.0-1.0, where 0.5,0.5 = center)
    bool flipH, flipV;   // Horizontal/vertical flip

public:
    Sprite() : bmp(nullptr), sx(0), sy(0), srcX(0), srcY(0), xoff(0), yoff(0), graph(nullptr),
               x(0.0f), y(0.0f), alpha(255.0f), scale(1.0f), rotation(0.0f),
               pivotX(0.5f), pivotY(0.5f), flipH(false), flipV(false) {}

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
    void setOffset(int offx, int offy) { xoff = offx; yoff = offy; }

    // Animatable property accessors
    float getX() const { return x; }
    float getY() const { return y; }
    float getAlpha() const { return alpha; }
    float getScale() const { return scale; }
    float getRotation() const { return rotation; }
    float getPivotX() const { return pivotX; }
    float getPivotY() const { return pivotY; }
    bool getFlipH() const { return flipH; }
    bool getFlipV() const { return flipV; }

    void setX(float newX) { x = newX; }
    void setY(float newY) { y = newY; }
    void setAlpha(float newAlpha) { alpha = newAlpha; }
    void setScale(float newScale) { scale = newScale; }
    void setRotation(float newRotation) { rotation = newRotation; }
    void setPivot(float newPivotX, float newPivotY) { pivotX = newPivotX; pivotY = newPivotY; }
    void setFlipH(bool flip) { flipH = flip; }
    void setFlipV(bool flip) { flipV = flip; }
    void setPos(float newX, float newY) { x = newX; y = newY; }

    // Pointer accessors for motion system (allows direct write)
    float* xPtr() { return &x; }
    float* yPtr() { return &y; }
    float* alphaPtr() { return &alpha; }
    float* scalePtr() { return &scale; }
    float* rotationPtr() { return &rotation; }

    // Temporary friend class or public access until refactoring is complete
    friend class Graph;
    friend class Floor;
    friend class Scene;
};
