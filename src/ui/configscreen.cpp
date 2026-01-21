#include "main.h"
#include "appdata.h"
#include "appconsole.h"
#include "configscreen.h"
#include "configdata.h"
#include <SDL.h>
#include <cstdio>
#include <cstring>

ConfigScreen::ConfigScreen()
    : state(ConfigState::Normal), selectedOption(0), waitingForKey(-1), tempRenderMode(0),
      upPressed(false), downPressed(false), leftPressed(false), rightPressed(false)
{
    std::memset(tempKeys, 0, sizeof(tempKeys));
}

int ConfigScreen::init()
{
    GameState::init();
    GameState::initSharedBackground();
    
    for (int player = 0; player < 2; player++)
    {
        tempKeys[player][0] = gameinf.getKeys()[player].left;
        tempKeys[player][1] = gameinf.getKeys()[player].right;
        tempKeys[player][2] = gameinf.getKeys()[player].shoot;
    }
    
    tempRenderMode = globalmode;

    if ( fontLoader.load ( "assets/fonts/monospaced_10.fnt" ) )
    {
        fontBmp.init ( &appGraph, "assets/fonts/monospaced_10.png", 0, 0 );
        fontRenderer.init ( &appGraph, &fontLoader, &fontBmp);
        fontRenderer.setScale(2.0f); // Scale 2x for readability
    }
    
    return 1;
}

GameState* ConfigScreen::moveAll()
{
    GameState::updateScrollingBackground();
    
    if (state == ConfigState::WaitingKey)
    {
        const Uint8* keystate = SDL_GetKeyboardState(nullptr);
        
        for (int i = (int)SDL_SCANCODE_A; i < (int)SDL_NUM_SCANCODES; i++)
        {
            SDL_Scancode scancode = static_cast<SDL_Scancode>(i);
            if (keystate[scancode])
            {
                int player = selectedOption / 3;
                int keyIndex = selectedOption % 3;
                tempKeys[player][keyIndex] = scancode;
                
                state = ConfigState::Normal;
                SDL_Delay(200);
                break;
            }
        }
        
        if (appInput.key(SDL_SCANCODE_ESCAPE))
        {
            state = ConfigState::Normal;
            SDL_Delay(200);
        }
    }
    else
    {
        if (appInput.key(SDL_SCANCODE_UP))
        {
            if (!upPressed)
            {
                selectedOption--;
                if (selectedOption < 0) selectedOption = 6;
                upPressed = true;
            }
        }
        else upPressed = false;
        
        if (appInput.key(SDL_SCANCODE_DOWN))
        {
            if (!downPressed)
            {
                selectedOption++;
                if (selectedOption > 6) selectedOption = 0;
                downPressed = true;
            }
        }
        else downPressed = false;
        
        if (appInput.key(SDL_SCANCODE_RETURN))
        {
            if (selectedOption < 6)
            {
                state = ConfigState::WaitingKey;
                waitingForKey = selectedOption;
                SDL_Delay(200);
            }
        }
        
        if (selectedOption == 6)
        {
            if (appInput.key(SDL_SCANCODE_LEFT))
            {
                if (!leftPressed)
                {
                    tempRenderMode = RENDERMODE_NORMAL;
                    leftPressed = true;
                }
            }
            else leftPressed = false;
                
            if (appInput.key(SDL_SCANCODE_RIGHT))
            {
                if (!rightPressed)
                {
                    tempRenderMode = RENDERMODE_EXCLUSIVE;
                    rightPressed = true;
                }
            }
            else rightPressed = false;
        }
        
        if (appInput.key(SDL_SCANCODE_F1))
        {
            saveConfiguration();
            return new Menu();
        }
        
        if (appInput.key(SDL_SCANCODE_ESCAPE))
        {
            cancelConfiguration();
            return new Menu();
        }
    }
    
    return nullptr;
}

int ConfigScreen::drawAll()
{
    GameState::drawScrollingBackground();
    drawUI();
    
    // Finalize render (debug overlay, console, flip)
    finalizeRender();
    
    return 1;
}

void ConfigScreen::drawUI()
{
    int y = 50;
    int xLabel = 80;
    int xKey = 320;
    int lineHeight = 25;
    
    drawText("CONFIGURATION", 220, 10, false);
    
    SDL_SetRenderDrawColor(appGraph.getRenderer(), 255, 255, 255, 255);
    SDL_RenderDrawLine(appGraph.getRenderer(), 60, 40, 580, 40);
    
    drawText("PLAYER 1:", xLabel, y, false);
    y += lineHeight;
    
    drawText("Left:", xLabel + 20, y, selectedOption == 0);
    drawKeyName(tempKeys[0][0], xKey, y);
    y += lineHeight;
    
    drawText("Right:", xLabel + 20, y, selectedOption == 1);
    drawKeyName(tempKeys[0][1], xKey, y);
    y += lineHeight;
    
    drawText("Shoot:", xLabel + 20, y, selectedOption == 2);
    drawKeyName(tempKeys[0][2], xKey, y);
    y += lineHeight + 10;
    
    drawText("PLAYER 2:", xLabel, y, false);
    y += lineHeight;
    
    drawText("Left:", xLabel + 20, y, selectedOption == 3);
    drawKeyName(tempKeys[1][0], xKey, y);
    y += lineHeight;
    
    drawText("Right:", xLabel + 20, y, selectedOption == 4);
    drawKeyName(tempKeys[1][1], xKey, y);
    y += lineHeight;
    
    drawText("Shoot:", xLabel + 20, y, selectedOption == 5);
    drawKeyName(tempKeys[1][2], xKey, y);
    y += lineHeight + 10;
    
    SDL_SetRenderDrawColor(appGraph.getRenderer(), 255, 255, 255, 255);
    SDL_RenderDrawLine(appGraph.getRenderer(), 60, y, 580, y);
    y += 10;
    
    drawText("Screen Mode:", xLabel, y, selectedOption == 6);
    const char* modeText = (tempRenderMode == RENDERMODE_NORMAL) ? "Windowed" : "Fullscreen";
    drawText(modeText, xKey, y, false);
    y += lineHeight + 10;
    
    SDL_SetRenderDrawColor(appGraph.getRenderer(), 255, 255, 255, 255);
    SDL_RenderDrawLine(appGraph.getRenderer(), 60, y, 580, y);
    y += 10;
    
    if (state == ConfigState::WaitingKey)
    {
        drawText("Press a key... (ESC to cancel)", 120, y, false);
    }
    else
    {
        drawText("Arrows: Navigate  |  ENTER: Change key", 80, y, false);
        drawText("F1: Save  |  ESC: Cancel", 160, y + 15, false);
    }
}

void ConfigScreen::drawText(const char* text, int x, int y, bool selected)
{
    if (selected)
    {
        SDL_SetRenderDrawColor(appGraph.getRenderer(), 255, 255, 0, 255);
        SDL_Rect selRect = { x - 10, y+10, 5, 16 };
        SDL_RenderFillRect(appGraph.getRenderer(), &selRect);
    }
    fontRenderer.text(text, x, y);
}

void ConfigScreen::drawDebugOverlay()
{
    AppData& appData = AppData::instance();
    
    if (!appData.debugMode)
    {
        // Clear overlay when debug mode is disabled
        textOverlay.clear();
        return;
    }
    
    // Call base class to populate default section
    GameState::drawDebugOverlay();
    
    char txt[256];
    std::snprintf(txt, sizeof(txt), "Selected = %d  State = %d", 
            selectedOption, (int)state);
    textOverlay.addText(txt);
}

void ConfigScreen::drawKeyName(SDL_Scancode key, int x, int y)
{
    const char* name = getKeyName(key);
    fontRenderer.text(name, x, y);
}

const char* ConfigScreen::getKeyName(SDL_Scancode scancode) const
{
    const char* name = SDL_GetScancodeName(scancode);
    return name ? name : "Unknown";
}

void ConfigScreen::saveConfiguration()
{
    AppData& appData = AppData::instance();
    
    for (int player = 0; player < 2; player++)
    {
        appData.getKeys()[player].setLeft(tempKeys[player][0]);
        appData.getKeys()[player].setRight(tempKeys[player][1]);
        appData.getKeys()[player].setShoot(tempKeys[player][2]);
    }
    
    int oldMode = globalmode;
    globalmode = tempRenderMode;
    
    if (oldMode != globalmode)
    {
        bool isFullscreen = (globalmode == RENDERMODE_EXCLUSIVE);
        appData.graph.setFullScreen(isFullscreen);
    }
    
    appData.config.save();
}

void ConfigScreen::cancelConfiguration()
{
}

int ConfigScreen::release()
{
    fontBmp.release();
    fontRenderer.release();
    
    return 1;
}
