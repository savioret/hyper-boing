#include <SDL.h>
#include <cstdio>
#include "app.h"
#include "appdata.h"
#include "appconsole.h"
#include "sprite.h"
#include "graph.h"
#include "pang.h"

GameState::GameState()
    : gameSpeed(0), fps(0), fpsv(0), active(true), pause(false), 
      difTime1(0), difTime2(0), time1(0), time2(0),
      frameStatus(0), frameCount(0), frameTick(0), lastFrameTick(0)
{		
}

int GameState::init()
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
    
    // Initialize frame timing
    frameStatus = 0;
    frameCount = 0;
    frameTick = 0;
    lastFrameTick = 0;
    
    return 1;
}

void GameState::drawDebugOverlay()
{
    AppData& appData = AppData::instance();
    
    if (!appData.debugMode) return;
    
    char txt[256];
    int y = 20;
    int lineHeight = 20;
    int width = 400;
    int height = 300;
    
    SDL_SetRenderDrawBlendMode(appData.graph.getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(appData.graph.getRenderer(), 0, 0, 0, 180);
    SDL_Rect bgRect = {10, 10, width, height};
    SDL_RenderFillRect(appData.graph.getRenderer(), &bgRect);
    SDL_SetRenderDrawBlendMode(appData.graph.getRenderer(), SDL_BLENDMODE_NONE);
    
    std::snprintf(txt, sizeof(txt), "FPS = %d  FPSVIRT = %d", fps, fpsv);
    appData.graph.text(txt, 20, y);
    y += lineHeight;
    
    std::snprintf(txt, sizeof(txt), "Paused = %s  Active = %s", 
            pause ? "YES" : "NO",
            active ? "YES" : "NO");
    appData.graph.text(txt, 20, y);
}

/**
 * Final render step - adds debug overlay, console overlay, and flips
 * Call this at the end of drawAll() in derived classes
 */
void GameState::finalizeRender()
{
    AppData& appData = AppData::instance();
    
    // Draw debug overlay if enabled
    drawDebugOverlay();
    
    // Render AppConsole overlay (always last, on top of everything)
    AppConsole::instance().render();
    
    // Present the rendered frame
    appData.graph.flip();
}

void GameState::initSharedBackground()
{
    AppData& appData = AppData::instance();
    
    if (!appData.backgroundInitialized)
    {
        appData.sharedBackground = std::make_unique<Sprite>();
        appData.sharedBackground->init(&appData.graph, "graph/titleback.png", 0, 0);
        appData.graph.setColorKey(appData.sharedBackground->getBmp(), 0xFF0000);
        appData.scrollX = 0.0f;
        appData.scrollY = (float)appData.sharedBackground->getHeight();
        appData.backgroundInitialized = true;
    }
}

void GameState::updateScrollingBackground()
{
    AppData& appData = AppData::instance();
    
    if (!appData.backgroundInitialized || !appData.sharedBackground) return;
    
    if (appData.scrollX < appData.sharedBackground->getWidth()) 
        appData.scrollX += 0.5f;
    else 
        appData.scrollX = 0.0f;

    if (appData.scrollY > 0) 
        appData.scrollY -= 0.5f;
    else 
        appData.scrollY = (float)appData.sharedBackground->getHeight();
}

void GameState::drawScrollingBackground()
{
    AppData& appData = AppData::instance();
    
    if (!appData.backgroundInitialized || !appData.sharedBackground) return;
    
    int i, j;
    SDL_Rect rc, rcbx, rcby, rcq;
    int sx = (int)appData.scrollX;
    int sy = (int)appData.scrollY;
    
    rc.x = sx;
    rc.w = appData.sharedBackground->getWidth() - sx;
    rc.y = 0;
    rc.h = sy;
    
    rcbx.x = 0;
    rcbx.w = sx;
    rcbx.y = 0;
    rcbx.h = sy;

    rcby.x = sx;
    rcby.w = appData.sharedBackground->getWidth() - sx;
    rcby.y = sy;
    rcby.h = appData.sharedBackground->getHeight() - sy;

    rcq.x = 0;
    rcq.w = sx;
    rcq.y = sy;
    rcq.h = appData.sharedBackground->getHeight() - sy;
    
    for (i = 0; i < 4; i++)
        for (j = 0; j < 5; j++)
        {
            appData.graph.draw(appData.sharedBackground->getBmp(), &rc, 
                appData.sharedBackground->getWidth() * i, 
                (appData.sharedBackground->getHeight() * j) + appData.sharedBackground->getHeight() - sy);
            appData.graph.draw(appData.sharedBackground->getBmp(), &rcbx, 
                (appData.sharedBackground->getWidth() * i) + rc.w, 
                (appData.sharedBackground->getHeight() * j) + appData.sharedBackground->getHeight() - sy);
            appData.graph.draw(appData.sharedBackground->getBmp(), &rcby, 
                appData.sharedBackground->getWidth() * i, 
                appData.sharedBackground->getHeight() * j);
            appData.graph.draw(appData.sharedBackground->getBmp(), &rcq, 
                (appData.sharedBackground->getWidth() * i) + appData.sharedBackground->getWidth() - sx, 
                appData.sharedBackground->getHeight() * j);
        }
}

void GameState::releaseSharedBackground()
{
    AppData& appData = AppData::instance();
    
    if (appData.backgroundInitialized && appData.sharedBackground)
    {
        appData.sharedBackground->release();
        appData.sharedBackground.reset();
        appData.backgroundInitialized = false;
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
GameState* GameState::doTick()
{
    AppData& appData = AppData::instance();
    
    if (appData.goBack)
    {
        appData.goBack = false;
        appData.isMenu() = true;
        return (GameState*) new Menu;
    }

    if (frameStatus == 0)
    {
        time1 = SDL_GetTicks();
        difTime2 = time1 - time2;
        if (difTime2 < gameSpeed) return nullptr;
        time2 = time1;
        difTime1 += difTime2;
        frameStatus = 1;
        return nullptr;
    }

    if (frameStatus == 1)
    {
        if (difTime1 < gameSpeed)
        {
            frameStatus = 2;
            return nullptr;
        }		
        GameState* newscreen = moveAll();
        difTime1 -= gameSpeed;
        return newscreen;
    }
    
    if (frameStatus == 2)
    {
        drawAll();
        frameStatus = 0;
        frameTick = SDL_GetTicks();
        if (frameTick - lastFrameTick > 1000)
        {
            fps = frameCount;
            frameCount = 0;
            lastFrameTick = frameTick;
        }
        else
            frameCount++;
    }

    return nullptr;
}

/**
 * If we pause, we need to update these data points so that the 
 * doTick function behaves correctly once the game is resumed.
 */
void GameState::doPause()
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
void GameState::setGameSpeed(int speed)
{
    gameSpeed = 1000 / speed;
}

void GameState::setActive(bool b)
{
    active = b;
}

void GameState::setPause(bool b)
{
    pause = b;
}

