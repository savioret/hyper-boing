#include "pang.h"
#include "appdata.h"
#include "appconsole.h"
#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include <cstring>

int Menu::initBitmaps()
{
    // Load three layered title images
    bmp.title_boing.init(&appGraph, "graph/title_boing.png", 0, 0);
    //appGraph.setColorKey(bmp.title_boing.getBmp(), 0x00FF00);
    
    bmp.title_hyper.init(&appGraph, "graph/title_hyper.png", 0, 0);
    //appGraph.setColorKey(bmp.title_hyper.getBmp(), 0x00FF00);
    
    bmp.title_bg.init(&appGraph, "graph/title_bg.png", 0, 0);
    //appGraph.setColorKey(bmp.title_bg.getBmp(), 0x00FF00);
    
    // Initialize shared background
    GameState::initSharedBackground();

    // Load BMFont for menu
    if (fontLoader.load("graph/font/thickfont_grad_64.fnt"))
    {
        bmp.menuFont.init(&appGraph, "graph/font/thickfont_grad_64.png", 0, 0);
        fontRenderer.init(&appGraph, &fontLoader, &bmp.menuFont);
    }

    return 1;
}

Menu::Menu()
    : xPos(0), yPos(0), selectedOption(0), visible(false), blinkCounter(0),
      upPressed(false), downPressed(false), enterPressed(false),
      boingY(-300), hyperX(-400), bgAlpha(0), animComplete(false)
{
}

int Menu::init()
{
    GameState::init();
    
    gameinf.isMenu() = true;
    initBitmaps();
    
    // Legacy position (kept for compatibility)
    xPos = 180;
    yPos = -300;
    
    // Initialize layered title animation
    boingY = -300;      // Starts above screen
    hyperX = -400;      // Starts off-screen to the left
    bgAlpha = 0;        // Starts fully transparent
    animComplete = false;

    selectedOption = 0; // PLAY
    
    visible = false;
    blinkCounter = 30;

    CloseMusic();
    OpenMusic("music/menu.ogg");
    PlayMusic();

    return 1;
}

int Menu::release()
{
    bmp.title_boing.release();
    bmp.title_hyper.release();
    bmp.title_bg.release();
    bmp.menuFont.release();
    fontRenderer.release();

    CloseMusic();
    
    return 1;
}

void Menu::drawTitleLayers()
{
    int centerX = (RES_X - bmp.title_bg.getWidth()) / 2;
    int targetY = 40;
    
    // Layer 1 (back): title_bg - fades in
    if (bgAlpha > 0)
    {
        SDL_SetTextureAlphaMod(bmp.title_bg.getBmp(), (Uint8)bgAlpha);
        appGraph.drawClipped(&bmp.title_bg, centerX, targetY);
    }

    // Layer 2 (front): title_boing - drops from top
    if (boingY > -300)
    {
        appGraph.drawClipped(&bmp.title_boing, centerX+30, boingY);
    }

    // Layer 3 (middle): title_hyper - slides from left
    if ( hyperX > -400 )
    {
        appGraph.drawClipped ( &bmp.title_hyper, hyperX, targetY+30 );
    }

}

void Menu::drawTitle()
{
    // Legacy method - now calls drawTitleLayers
    drawTitleLayers();
}

void Menu::drawMenu()
{
    int menuStartY = 320;
    int spacing = 50;
    
    if (fontRenderer.getFont() && fontRenderer.getFontTexture())
    {
        const char* options[3] = {"NEW GAME", "OPTIONS", "EXIT"};
        
        for (int i = 0; i < 3; i++)
        {
            int optionY = menuStartY + (spacing * i);
            int textWidth = fontRenderer.getTextWidth(options[i]);
            int textX = (RES_X - textWidth) / 2;
            
            if (selectedOption != i || visible)
            {
                fontRenderer.text(options[i], textX, optionY);
            }
        }
        
        if (visible)
        {
            int indicatorY = menuStartY + (spacing * selectedOption);
            int textWidth = fontRenderer.getTextWidth(options[selectedOption]);
            int textX = (RES_X - textWidth) / 2;
            int indicatorX = textX - 30;
            fontRenderer.text(">", indicatorX, indicatorY);
        }
    }
}

int Menu::drawAll()
{
    GameState::drawScrollingBackground();
    drawTitle();
    drawMenu();
    
    // Finalize render (debug overlay, console, flip)
    finalizeRender();
    
    return 1;
}

void Menu::drawDebugOverlay()
{
    AppData& appData = AppData::instance();
    
    if (!appData.debugMode) return;
    GameState::drawDebugOverlay();
    char txt[256];
    int y = 80;
    int lineHeight = 20;
    std::snprintf(txt, sizeof(txt), "Title Boing Y = %d  Hyper X = %d  BG Alpha = %d", boingY, hyperX, bgAlpha);
    appData.graph.text(txt, 20, y);
    y += lineHeight;
    std::snprintf(txt, sizeof(txt), "AnimComplete = %s  Selected = %d", animComplete ? "YES" : "NO", selectedOption);
    appData.graph.text(txt, 20, y);
    y += lineHeight;
    std::snprintf(txt, sizeof(txt), "Scroll X=%d Y=%d", (int)appData.scrollX, (int)appData.scrollY);
    appData.graph.text(txt, 20, y);
}

GameState* Menu::moveAll()
{
    if (blinkCounter > 0) blinkCounter--;
    else blinkCounter = 30;

    GameState::updateScrollingBackground();

    // Animate layered title elements
    if (!animComplete)
    {
        // Phase 1: title_boing drops from top (fast)
        if (boingY < 120)
        {
            boingY += 10;
            if (boingY >= 120 )
            {
                boingY = 120;
            }
        }
        // Phase 2: title_hyper slides from left (after boing settles)
        else if (hyperX < (RES_X - bmp.title_hyper.getWidth()) / 2)
        {
            hyperX += 15;
            if (hyperX >= (RES_X - bmp.title_hyper.getWidth()) / 2)
            {
                hyperX = (RES_X - bmp.title_hyper.getWidth()) / 2;
            }
        }
        // Phase 3: title_bg fades in (after hyper arrives)
        else if (bgAlpha < 255)
        {
            bgAlpha += 5;
            if (bgAlpha >= 255)
            {
                bgAlpha = 255;
                animComplete = true;
            }
        }
    }
    
    // Legacy animation (kept for compatibility)
    if (yPos < 120) yPos += 10;
    else if (!blinkCounter) visible = !visible;

    // Enable menu interaction only after animations complete
    if (animComplete && yPos >= 50)
    {
        if (appInput.key(SDL_SCANCODE_UP) || appInput.key(gameinf.getKeys()[PLAYER1].left))
        {
            if (!upPressed)
            {
                selectedOption--;
                if (selectedOption < 0) selectedOption = 2;
                upPressed = true;
            }
        }
        else upPressed = false;
            
        if (appInput.key(SDL_SCANCODE_DOWN) || appInput.key(gameinf.getKeys()[PLAYER1].right))
        {
            if (!downPressed)
            {
                selectedOption++;
                if (selectedOption > 2) selectedOption = 0;
                downPressed = true;
            }
        }
        else downPressed = false;
        
        if (appInput.key(SDL_SCANCODE_RETURN) || appInput.key(gameinf.getKeys()[PLAYER1].shoot))
        {
            if (!enterPressed)
            {
                switch (selectedOption)
                {
                    case 0: return new SelectSync();
                    case 1: return new ConfigScreen();
                    case 2: quit = 1; return nullptr;
                }
                enterPressed = true;
            }
        }
        else enterPressed = false;
    }
    
    return nullptr;
}