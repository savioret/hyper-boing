#pragma once

#include <SDL.h>
#include "bmfont.h"
#include "appdata.h"
#include "audiomanager.h"

class Sprite;

// Convenience macros for global singletons - available globally
#define appGraph AppData::instance().graph
#define appInput AppData::instance().input
#define appAudio AudioManager::instance()

/**
 * GameState class
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
class GameState
{
protected:
    int gameSpeed; // frames per second in milliseconds
    int fps, fpsv;
    bool active; // window active?
    bool pause;
    int difTime1, difTime2;
    long time1, time2;
    
    // Frame timing variables (previously static in doTick)
    int frameStatus;
    int frameCount;
    long frameTick;
    long lastFrameTick;
    
    // BMFont for the menu
    BMFontLoader fontLoader;
    BMFontRenderer fontRenderer;

public:
    GameState();
    virtual ~GameState() {}

    GameState* doTick();
    void doPause();
    void setGameSpeed(int speed);
    void setPause(bool b);
    void setActive(bool b);
    bool isActive() const { return active; }
    bool isPaused() const { return pause; }

    // Virtual methods for derived classes
    virtual int init();
    virtual GameState* moveAll() = 0;
    virtual int drawAll() = 0;
    virtual int release() = 0;
    
    // Final render step - called after drawAll() to add overlays and flip
    void finalizeRender();
    
    // Shared background functions (now use AppData)
    static void initSharedBackground();
    static void updateScrollingBackground();
    static void drawScrollingBackground();
    static void releaseSharedBackground();
    
    // Debug overlay - virtual so each screen can customize it
    virtual void drawDebugOverlay();
};