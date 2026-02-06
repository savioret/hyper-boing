#include "../main.h"
#include "appdata.h"
#include "appconsole.h"
#include "configscreen.h"
#include "configdata.h"
#include <SDL.h>
#include <cstdio>
#include <cstring>

ConfigScreen::ConfigScreen()
    : state(ConfigState::Normal), selectedOption(0), waitingForKey(-1), tempRenderMode(0),
      upPressed(false), downPressed(false), leftPressed(false), rightPressed(false),
      enterPressed(false), escapePressed(false), editModeDelay(0.0f), initialDelay(INITIAL_DELAY_TIME),
      postConfigDelay(0.0f)
{
    std::memset(tempKeys, 0, sizeof(tempKeys));
}

int ConfigScreen::init()
{
    GameState::init();
    GameState::initSharedBackground();
    
    for (int player = 0; player < 2; player++)
    {
        tempKeys[player][0] = gameinf.getKeys(player).getLeft();
        tempKeys[player][1] = gameinf.getKeys(player).getRight();
        tempKeys[player][2] = gameinf.getKeys(player).getShoot();
        tempKeys[player][3] = gameinf.getKeys(player).getUp();
        tempKeys[player][4] = gameinf.getKeys(player).getDown();
    }
    
    tempRenderMode = globalmode;

    if ( fontLoader.load ( "assets/fonts/monospaced_10.fnt" ) )
    {
        fontBmp.init ( &appGraph, "assets/fonts/monospaced_10.png", 0, 0 );
        fontRenderer.init ( &appGraph, &fontLoader, &fontBmp);
        fontRenderer.setScale(2.0f); // Scale 2x for readability
    }
    
    // Setup custom modal overlay (centered on screen, large text)
    configModalOverlay.init(&appGraph);
    configModalOverlay.addSection("modal", 100, 150, 440, 120, 10, 30, 240);
    configModalOverlay.getSection("modal").setFont(&fontRenderer);
    
    return 1;
}

GameState* ConfigScreen::moveAll(float dt)
{
    GameState::updateScrollingBackground();
    
    // Handle initial delay to prevent capturing ENTER from menu
    if (initialDelay > 0.0f)
    {
        initialDelay -= dt;
        // Don't process any input during initial delay
        return nullptr;
    }
    
    // Handle post-configuration delay to prevent navigation keys just bound
    if (postConfigDelay > 0.0f)
    {
        postConfigDelay -= dt;
        // Only allow ESC and F1 during post-config delay (exit options)
        if (appInput.key(SDL_SCANCODE_F1))
        {
            saveConfiguration();
            return new Menu();
        }
        
        if (appInput.key(SDL_SCANCODE_ESCAPE))
        {
            if (!escapePressed)
            {
                cancelConfiguration();
                escapePressed = true;
                return new Menu();
            }
        }
        else escapePressed = false;
        
        // Don't process navigation or ENTER during post-config delay
        return nullptr;
    }
    
    if (state == ConfigState::EnteringEdit)
    {
        // Wait for ENTER key to be released before accepting new key input
        editModeDelay -= dt;
        
        if (editModeDelay <= 0.0f && !appInput.key(SDL_SCANCODE_RETURN))
        {
            // ENTER has been released and delay has passed, now ready to capture new key
            state = ConfigState::WaitingKey;
        }
        
        // Allow ESC to cancel immediately
        if (appInput.key(SDL_SCANCODE_ESCAPE))
        {
            if (!escapePressed)
            {
                state = ConfigState::Normal;
                postConfigDelay = POST_CONFIG_DELAY_TIME;  // Add delay after canceling
                escapePressed = true;
            }
        }
        else escapePressed = false;
    }
    else if (state == ConfigState::WaitingKey)
    {
        const Uint8* keystate = SDL_GetKeyboardState(nullptr);
        
        // Skip ENTER and ESCAPE from being captured as key bindings
        for (int i = (int)SDL_SCANCODE_A; i < (int)SDL_NUM_SCANCODES; i++)
        {
            SDL_Scancode scancode = static_cast<SDL_Scancode>(i);
            
            // Skip ENTER and ESCAPE
            if (scancode == SDL_SCANCODE_RETURN || scancode == SDL_SCANCODE_ESCAPE)
                continue;
            
            if (keystate[scancode])
            {
                int player = selectedOption / 5;
                int keyIndex = selectedOption % 5;
                tempKeys[player][keyIndex] = scancode;
                
                state = ConfigState::Normal;
                postConfigDelay = POST_CONFIG_DELAY_TIME;  // Add delay after key configured
                break;
            }
        }
        
        if (appInput.key(SDL_SCANCODE_ESCAPE))
        {
            if (!escapePressed)
            {
                state = ConfigState::Normal;
                postConfigDelay = POST_CONFIG_DELAY_TIME;  // Add delay after canceling
                escapePressed = true;
            }
        }
        else escapePressed = false;
    }
    else // ConfigState::Normal
    {
        if (appInput.key(SDL_SCANCODE_UP))
        {
            if (!upPressed)
            {
                selectedOption--;
                if (selectedOption < 0) selectedOption = 10;
                upPressed = true;
            }
        }
        else upPressed = false;
        
        if (appInput.key(SDL_SCANCODE_DOWN))
        {
            if (!downPressed)
            {
                selectedOption++;
                if (selectedOption > 10) selectedOption = 0;
                downPressed = true;
            }
        }
        else downPressed = false;
        
        if (appInput.key(SDL_SCANCODE_RETURN))
        {
            if (!enterPressed)
            {
                if (selectedOption < 10)
                {
                    state = ConfigState::EnteringEdit;
                    waitingForKey = selectedOption;
                    editModeDelay = EDIT_MODE_DELAY_TIME;
                }
                enterPressed = true;
            }
        }
        else enterPressed = false;
        
        if (selectedOption == 10)
        {
            if (appInput.key(SDL_SCANCODE_LEFT))
            {
                if (!leftPressed)
                {
                    // Cycle backwards through modes
                    tempRenderMode--;
                    if (tempRenderMode < RENDERMODE_NORMAL)
                        tempRenderMode = RENDERMODE_WINDOWED_2X;
                    leftPressed = true;
                }
            }
            else leftPressed = false;
                
            if (appInput.key(SDL_SCANCODE_RIGHT))
            {
                if (!rightPressed)
                {
                    // Cycle forwards through modes
                    tempRenderMode++;
                    if (tempRenderMode > RENDERMODE_WINDOWED_2X)
                        tempRenderMode = RENDERMODE_NORMAL;
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
            if (!escapePressed)
            {
                cancelConfiguration();
                escapePressed = true;
                return new Menu();
            }
        }
        else escapePressed = false;
    }
    
    return nullptr;
}

int ConfigScreen::drawAll()
{
    GameState::drawScrollingBackground();
    drawUI();
    
    // Draw custom modal overlay if in edit mode (not the debug overlay)
    if (state == ConfigState::EnteringEdit || state == ConfigState::WaitingKey)
    {
        drawConfigModal();
    }
    
    // Finalize render (debug overlay, console, flip)
    finalizeRender();
    
    return 1;
}

void ConfigScreen::drawConfigModal()
{
    // Draw semi-transparent dark overlay backdrop
    SDL_SetRenderDrawBlendMode(appGraph.getRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(appGraph.getRenderer(), 0, 0, 0, 180);
    SDL_Rect backdrop = { 0, 0, RES_X, RES_Y };
    SDL_RenderFillRect(appGraph.getRenderer(), &backdrop);
    
    // Draw modal box border (background is drawn by TextOverlay)
    SDL_SetRenderDrawColor(appGraph.getRenderer(), 200, 200, 220, 255);
    SDL_Rect modalBox = { 100, 150, 440, 120 };
    SDL_RenderDrawRect(appGraph.getRenderer(), &modalBox);
    
    // Prepare text content
    const char* actionNames[] = {
        "PLAYER 1 - LEFT",
        "PLAYER 1 - RIGHT",
        "PLAYER 1 - SHOOT",
        "PLAYER 1 - UP",
        "PLAYER 1 - DOWN",
        "PLAYER 2 - LEFT",
        "PLAYER 2 - RIGHT",
        "PLAYER 2 - SHOOT",
        "PLAYER 2 - UP",
        "PLAYER 2 - DOWN"
    };
    
    // Clear and populate modal overlay
    configModalOverlay.clear("modal");
    configModalOverlay.addTextFS("modal", "CONFIGURING: %s", actionNames[selectedOption]);
    configModalOverlay.addText("", "modal");  // Empty line for spacing
    
    if (state == ConfigState::WaitingKey)
    {
        configModalOverlay.addText("Press any key...", "modal");
        configModalOverlay.addText("(ESC to cancel)", "modal");
    }
    else // EnteringEdit
    {
        configModalOverlay.addText("Release ENTER...", "modal");
    }
    
    // Render the modal overlay
    configModalOverlay.render();
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
    y += lineHeight;

    drawText("Up:", xLabel + 20, y, selectedOption == 3);
    drawKeyName(tempKeys[0][3], xKey, y);
    y += lineHeight;

    drawText("Down:", xLabel + 20, y, selectedOption == 4);
    drawKeyName(tempKeys[0][4], xKey, y);
    y += lineHeight + 10;
    
    drawText("PLAYER 2:", xLabel, y, false);
    y += lineHeight;
    
    drawText("Left:", xLabel + 20, y, selectedOption == 5);
    drawKeyName(tempKeys[1][0], xKey, y);
    y += lineHeight;

    drawText("Right:", xLabel + 20, y, selectedOption == 6);
    drawKeyName(tempKeys[1][1], xKey, y);
    y += lineHeight;

    drawText("Shoot:", xLabel + 20, y, selectedOption == 7);
    drawKeyName(tempKeys[1][2], xKey, y);
    y += lineHeight;

    drawText("Up:", xLabel + 20, y, selectedOption == 8);
    drawKeyName(tempKeys[1][3], xKey, y);
    y += lineHeight;

    drawText("Down:", xLabel + 20, y, selectedOption == 9);
    drawKeyName(tempKeys[1][4], xKey, y);
    y += lineHeight + 10;
    
    SDL_SetRenderDrawColor(appGraph.getRenderer(), 255, 255, 255, 255);
    SDL_RenderDrawLine(appGraph.getRenderer(), 60, y, 580, y);
    y += 10;
    
    drawText("Screen Mode:", xLabel, y, selectedOption == 10);
    const char* modeText;
    if (tempRenderMode == RENDERMODE_NORMAL)
        modeText = "Windowed 1x";
    else if (tempRenderMode == RENDERMODE_WINDOWED_2X)
        modeText = "Windowed 2x";
    else
        modeText = "Fullscreen";
    drawText(modeText, xKey, y, false);
    y += lineHeight + 10;
    
    SDL_SetRenderDrawColor(appGraph.getRenderer(), 255, 255, 255, 255);
    SDL_RenderDrawLine(appGraph.getRenderer(), 60, y, 580, y);
    y += 10;
    
    // Instructions at bottom (only when not in edit mode)
    if (state == ConfigState::Normal)
    {
        drawText("Arrows: Navigate  |  ENTER: Change key", 80, y, false);
        drawText("F1: Save  |  ESC: Cancel", 160, y + 20, false);
    }
}

void ConfigScreen::drawText(const char* text, int x, int y, bool selected)
{
    if (selected)
    {
        SDL_SetRenderDrawColor(appGraph.getRenderer(), 255, 255, 0, 255);
        SDL_Rect selRect = { x - 10, y, 5, 16 };
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

    // Show current state name for easier debugging
    const char* stateNames[] = { "Normal", "EnteringEdit", "WaitingKey" };
    textOverlay.addTextF("Selected = %d  State = %s", selectedOption, stateNames[(int)state]);
    textOverlay.addTextF("editDelay=%.2f  initialDelay=%.2f  postCfgDelay=%.2f", 
                         editModeDelay, initialDelay, postConfigDelay);
}

void ConfigScreen::drawKeyName(SDL_Scancode key, int x, int y)
{
    const char* name = Keys::getKeyName(key);
    fontRenderer.text(name, x, y);
}

void ConfigScreen::saveConfiguration()
{
    AppData& appData = AppData::instance();
    
    for (int player = 0; player < 2; player++)
    {
        appData.getKeys(player).setLeft(tempKeys[player][0]);
        appData.getKeys(player).setRight(tempKeys[player][1]);
        appData.getKeys(player).setShoot(tempKeys[player][2]);
        appData.getKeys(player).setUp(tempKeys[player][3]);
        appData.getKeys(player).setDown(tempKeys[player][4]);
    }
    
    int oldMode = globalmode;
    globalmode = tempRenderMode;
    
    if (oldMode != globalmode)
    {
        // Update the mode in the graph instance
        appData.graph.setMode(globalmode);
        
        // Handle mode transitions
        if (globalmode == RENDERMODE_EXCLUSIVE)
        {
            // Switch to fullscreen
            appData.graph.setFullScreen(true);
        }
        else if (oldMode == RENDERMODE_EXCLUSIVE)
        {
            // Switch from fullscreen to windowed
            appData.graph.setFullScreen(false);
        }
        else
        {
            // Switching between windowed modes (1x <-> 2x)
            // Just resize the window
            int windowWidth = (globalmode == RENDERMODE_WINDOWED_2X) ? RES_X * 2 : RES_X;
            int windowHeight = (globalmode == RENDERMODE_WINDOWED_2X) ? RES_Y * 2 : RES_Y;
            appData.graph.setWindowSize(windowWidth, windowHeight);
        }
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
