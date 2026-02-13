#pragma once

#include <vector>
#include <SDL.h>

class Sprite;

/**
 * Sprite2D class
 *
 * Represents a positioned sprite instance with render properties.
 * Ideal for UI elements and tweening animations that need:
 * - Position (x, y) with pointer accessors for tweening
 * - Render properties (alpha, scale, rotation, flip)
 * - Frame selection from multiple sprites
 *
 * Note: For game entities that inherit from IGameObject, prefer
 * composition with ISprite classes to avoid position duplication.
 * Sprite2D is best suited for UI elements like MenuTitle.
 */
class Sprite2D
{
protected:
    std::vector<Sprite*> sprites;
    float x, y;
    int frame;
    bool visible;
    
    // Rendering properties
    float alpha;
    float scale;
    float rotation;
    float pivotX, pivotY;
    bool flipH, flipV;

public:
    Sprite2D();
    virtual ~Sprite2D() = default;

    // Sprite management
    void addSprite(Sprite* sprite);
    void clearSprites();
    Sprite* getCurrentSprite() const;

    // Properties
    void setPos(float X, float Y) { x = X; y = Y; }
    void setX(float val) { x = val; }
    void setY(float val) { y = val; }
    
    void setFrame(int f);
    void setVisible(bool v) { visible = v; }
    void setAlpha(float a) { alpha = a; }
    void setScale(float s) { scale = s; }
    void setRotation(float r) { rotation = r; }
    void setPivot(float px, float py) { pivotX = px; pivotY = py; }
    void setFlipH(bool f) { flipH = f; }
    void setFlipV(bool f) { flipV = f; }

    float getX() const { return x; }
    float getY() const { return y; }
    
    // Helper to get dimensions of current frame
    int getWidth() const;
    int getHeight() const;
    
    int getFrame() const { return frame; }
    bool isVisible() const { return visible; }
    float getAlpha() const { return alpha; }
    float getScale() const { return scale; }
    float getRotation() const { return rotation; }
    float getPivotX() const { return pivotX; }
    float getPivotY() const { return pivotY; }
    bool getFlipH() const { return flipH; }
    bool getFlipV() const { return flipV; }

    // Pointer accessors for tweening/animation systems
    float* xPtr() { return &x; }
    float* yPtr() { return &y; }
    float* alphaPtr() { return &alpha; }
    float* scalePtr() { return &scale; }
    float* rotationPtr() { return &rotation; }
};
