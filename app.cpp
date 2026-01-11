#include <SDL.h>
#include <cstdio>
#include "app.h"
#include "graph.h"
#include "pang.h"

// Initialize static variables
Sprite* App::sharedBackground = nullptr;
float App::scrollX = 0;
float App::scrollY = 0;
bool App::backgroundInitialized = false;
bool App::debugMode = false;

App::App()
    : gameSpeed(0), fps(0), fpsv(0), active(true), pause(false), 
      difTime1(0), difTime2(0), time1(0), time2(0)
{		
}

int App::init()
{
    active = true;
    pause = false;
    setGameSpeed(60);
    difTime1 = 0;
    difTime2 = gameSpeed;
    time1 = SDL_GetTicks() + gameSpeed;
    time2 = SDL_GetTicks();
    fps = 0;
    fpsv = 0;
    
    return 1;
}

void App::drawDebugOverlay()
{
    if (!debugMode) return;
    
    char cadena[256];
    int y = 20;
    int lineHeight = 20;
    int width = 400;
    int height = 300;
    
    SDL_SetRenderDrawBlendMode(graph.getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(graph.getRenderer(), 0, 0, 0, 180);
    SDL_Rect bgRect = {10, 10, width, height};
    SDL_RenderFillRect(graph.getRenderer(), &bgRect);
    SDL_SetRenderDrawBlendMode(graph.getRenderer(), SDL_BLENDMODE_NONE);
    
    sprintf(cadena, "FPS = %d  FPSVIRT = %d", fps, fpsv);
    graph.text(cadena, 20, y);
    y += lineHeight;
    
    sprintf(cadena, "Paused = %s  Active = %s", 
            pause ? "YES" : "NO",
            active ? "YES" : "NO");
    graph.text(cadena, 20, y);
}

void App::initSharedBackground()
{
    if (!backgroundInitialized)
    {
        sharedBackground = new Sprite();
        sharedBackground->init(&graph, "graph\\titleback.png", 0, 0);
        graph.setColorKey(sharedBackground->getBmp(), 0xFF0000);
        scrollX = 0.0f;
        scrollY = (float)sharedBackground->getHeight();
        backgroundInitialized = true;
    }
}

void App::updateScrollingBackground()
{
    if (!backgroundInitialized || !sharedBackground) return;
    
    if (scrollX < sharedBackground->getWidth()) scrollX += 0.5f;
    else scrollX = 0.0f;

    if (scrollY > 0) scrollY -= 0.5f;
    else scrollY = (float)sharedBackground->getHeight();
}

void App::drawScrollingBackground()
{
    if (!backgroundInitialized || !sharedBackground) return;
    
    int i, j;
    SDL_Rect rc, rcbx, rcby, rcq;
    int sx = (int)scrollX;
    int sy = (int)scrollY;
    
    rc.x = sx;
    rc.w = sharedBackground->getWidth() - sx;
    rc.y = 0;
    rc.h = sy;
    
    rcbx.x = 0;
    rcbx.w = sx;
    rcbx.y = 0;
    rcbx.h = sy;

    rcby.x = sx;
    rcby.w = sharedBackground->getWidth() - sx;
    rcby.y = sy;
    rcby.h = sharedBackground->getHeight() - sy;

    rcq.x = 0;
    rcq.w = sx;
    rcq.y = sy;
    rcq.h = sharedBackground->getHeight() - sy;
    
    for (i = 0; i < 4; i++)
        for (j = 0; j < 5; j++)
        {
            graph.draw(sharedBackground->getBmp(), &rc, sharedBackground->getWidth() * i, (sharedBackground->getHeight() * j) + sharedBackground->getHeight() - sy);
            graph.draw(sharedBackground->getBmp(), &rcbx, (sharedBackground->getWidth() * i) + rc.w, (sharedBackground->getHeight() * j) + sharedBackground->getHeight() - sy);
            graph.draw(sharedBackground->getBmp(), &rcby, sharedBackground->getWidth() * i, sharedBackground->getHeight() * j);
            graph.draw(sharedBackground->getBmp(), &rcq, (sharedBackground->getWidth() * i) + sharedBackground->getWidth() - sx, sharedBackground->getHeight() * j);
        }
}

void App::releaseSharedBackground()
{
    if (backgroundInitialized && sharedBackground)
    {
        sharedBackground->release();
        delete sharedBackground;
        sharedBackground = nullptr;
        backgroundInitialized = false;
    }
}

/**
 * This function is central to the game.
 * It manages the number of times the screen is painted, up to the set maximum,
 * and calculations are performed as many times as specified (gameSpeed variable).
 *
 * For example, if we want to render 60 frames per second using a counter,
 * we ensure that 60 calculations are performed. If time permits, we render 60 frames;
 * on slower systems, we render only as many as possible, but the "virtual" speed
 * of the game remains 60 fps.
 */
void* App::doTick()
{
    static short framestatus = 0;	
    static short cont = 0;
    static long tick, lasttick;

    if (goback)
    {
        goback = false;
        gameinf.isMenu() = true;
        return (App*) new Menu;
    }

    if (framestatus == 0)
    {
        time1 = SDL_GetTicks();
        difTime2 = time1 - time2;
        if (difTime2 < gameSpeed) return nullptr;
        time2 = time1;
        difTime1 += difTime2;
        framestatus = 1;
        return nullptr;
    }

    if (framestatus == 1)
    {
        if (difTime1 < gameSpeed)
        {
            framestatus = 2;
            return nullptr;
        }		
        void* newscreen = (App*) moveAll();
        difTime1 -= gameSpeed;
        return newscreen;
    }
    
    if (framestatus == 2)
    {
        drawAll();
        framestatus = 0;
        tick = SDL_GetTicks();
        if (tick - lasttick > 1000)
        {
            fps = cont;
            cont = 0;
            lasttick = tick;
        }
        else
            cont++;
    }

    return nullptr;
}

/**
 * If we pause, we need to update these data points so that the 
 * doTick function behaves correctly once the game is resumed.
 */
void App::doPause()
{	
    difTime1 = 0; 
    difTime2 = gameSpeed;
    time1 = SDL_GetTicks() + gameSpeed;
    time2 = SDL_GetTicks();
}

/**
 * Adjust the rendering speed in frames per second and convert it to 
 * the equivalent milliseconds per frame, which is the actual data needed.
 */
void App::setGameSpeed(int speed)
{
    gameSpeed = 1000 / speed;
}

void App::setActive(bool b)
{
    active = b;
}

void App::setPause(bool b)
{
    pause = b;
}

