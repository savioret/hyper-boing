#pragma once

#include <SDL.h>
#include "bmfont.h"

class Sprite;

/**
 * App class
 *
 * This class is used to create different instances of the current game screen.
 * It serves as the base class for several game modules.
 *
 * Virtual functions:
 * - init(): Each module has associated bitmaps and resources loaded upon object creation,
 *           optimizing system resource management.
 * - drawAll(): Renders all elements currently on the screen.
 * - moveAll(): Handles movements, input reading, collision detection, etc.
 */
class App
{
protected:
    int gameSpeed; // frames per second in milliseconds
    int fps, fpsv;
    bool active; // window active?
    bool pause;
    short int difTime1, difTime2;
    long time1, time2;
    // BMFont for the menu
    BMFontLoader fontLoader;
    BMFontRenderer fontRenderer;

public:
    // Shared scrolling background
    static Sprite* sharedBackground;
    static float scrollX, scrollY;
    static bool backgroundInitialized;
    
    // Global debug mode
    static bool debugMode;

    App();
    virtual ~App() {}

    void* doTick();
    void doPause();
    void setGameSpeed(int speed);
    void setPause(bool b);
    void setActive(bool b);
    bool isActive() const { return active; }
    bool isPaused() const { return pause; }

    // Virtual methods for derived classes
    virtual int init();
    virtual void* moveAll() = 0;
    virtual int drawAll() = 0;
    virtual int release() = 0;
    
    // Shared functions
    static void initSharedBackground();
    static void updateScrollingBackground();
    static void drawScrollingBackground();
    static void releaseSharedBackground();
    
    // Debug overlay - virtual so each screen can customize it
    virtual void drawDebugOverlay();
};