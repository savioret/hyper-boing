#include "main.h"
#include "appdata.h"
#include "appconsole.h"
#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include <cstring>

int Menu::initBitmaps()
{
    // Load layered title images
    bmp.title_boing.init(&appGraph, "assets/graph/ui/title_boing.png", 0, 0);
    //appGraph.setColorKey(bmp.title_boing.getBmp(), 0x00FF00);
    
    bmp.title_hyper.init(&appGraph, "assets/graph/ui/title_hyper.png", 0, 0);
    //appGraph.setColorKey(bmp.title_hyper.getBmp(), 0x00FF00);
    
    bmp.title_bg.init(&appGraph, "assets/graph/ui/title_bg.png", 0, 0);
    //appGraph.setColorKey(bmp.title_bg.getBmp(), 0x00FF00);

    bmp.title_bg_redball.init(&appGraph, "assets/graph/ui/title_bg_redball.png", 0, 0);
    bmp.title_bg_greenball.init(&appGraph, "assets/graph/ui/title_bg_greenball.png", 0, 0);
    bmp.title_bg_blueball.init(&appGraph, "assets/graph/ui/title_bg_blueball.png", 0, 0);
    
    // Initialize shared background
    GameState::initSharedBackground();

    // SIMPLIFIED: Load BMFont with one call - texture auto-loaded
    fontRenderer.loadFont(&appGraph, "assets/fonts/thickfont_grad_64.fnt");

    return 1;
}

Menu::Menu()
    : selectedOption(0), visible(false), blinkCounter(0),
      upPressed(false), downPressed(false), enterPressed(false),
      boingY(-300), hyperX(-400), bgAlpha(0), ballAlpha(0), animComplete(false)
{
}

int Menu::init()
{
    GameState::init();
    
    gameinf.isMenu() = true;
    initBitmaps();
    
    // Initialize layered title animation
    boingY = -300;      // Starts above screen
    hyperX = -400;      // Starts off-screen to the left
    bgAlpha = 0;        // Starts fully transparent
    ballAlpha = 0;      // Balls fade in after background

    animComplete = false;

    selectedOption = 0; // PLAY
    
    visible = false;
    blinkCounter = 30;

    CloseMusic();
    OpenMusic("assets/music/menu.ogg");
    PlayMusic();

    return 1;
}

int Menu::release()
{
    bmp.title_boing.release();
    bmp.title_hyper.release();
    bmp.title_bg.release();
    bmp.title_bg_redball.release();
    bmp.title_bg_greenball.release();
    bmp.title_bg_blueball.release();
    fontRenderer.release();  // BMFontRenderer handles its own cleanup

    CloseMusic();
    
    return 1;
}

void Menu::drawTitleLayers()
{
    const int centerX = (RES_X - bmp.title_bg.getWidth()) / 2;
    const int titleY = 5;

    // Background layer - fades in
    if (bgAlpha > 0)
    {
        SDL_SetTextureAlphaMod(bmp.title_bg.getBmp(), (Uint8)bgAlpha);
        appGraph.drawClipped(&bmp.title_bg, centerX, titleY + 60);
    }

    // Colored balls - fade in at static positions
    if (ballAlpha > 0)
    {
        SDL_SetTextureAlphaMod(bmp.title_bg_redball.getBmp(), (Uint8)ballAlpha);
        SDL_SetTextureAlphaMod(bmp.title_bg_greenball.getBmp(), (Uint8)ballAlpha);
        SDL_SetTextureAlphaMod(bmp.title_bg_blueball.getBmp(), (Uint8)ballAlpha);

        appGraph.drawClipped(&bmp.title_bg_redball, centerX - 10, titleY + 80);
        appGraph.drawClipped(&bmp.title_bg_blueball, centerX + 230, titleY + 180);
        appGraph.drawClipped(&bmp.title_bg_greenball, centerX + 350, titleY + 65);
    }

    // "HYPER" text - slides from left
    if (hyperX > -400)
    {
        appGraph.drawClipped(&bmp.title_hyper, hyperX - 30, titleY + 20);
    }

    // "BOING" text - drops from top
    if (boingY > -300)
    {
        appGraph.drawClipped(&bmp.title_boing, centerX + 30, boingY);
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
    
    if (!appData.debugMode)
    {
        // Clear overlay when debug mode is disabled
        textOverlay.clear();
        return;
    }
    
    // Call base class to populate default section
    GameState::drawDebugOverlay();

    textOverlay.addTextF("Title: boingY=%d hyperX=%d bgAlpha=%d ballAlpha=%d",
        boingY, hyperX, bgAlpha, ballAlpha);

    textOverlay.addTextF("AnimComplete=%s Selected=%d",
        animComplete ? "YES" : "NO", selectedOption);

    textOverlay.addTextF("Scroll X=%d Y=%d", (int)appData.scrollX, (int)appData.scrollY);
}

GameState* Menu::moveAll()
{
    if (blinkCounter > 0) blinkCounter--;
    else 
    {
        blinkCounter = 30;
        visible = !visible;
    }

    GameState::updateScrollingBackground();

    // Animate layered title elements
    if (!animComplete)
    {
        // Phase 1: title_boing drops from top
        if (boingY < 85)
        {
            boingY += 10;
            if (boingY >= 85)
            {
                boingY = 85;
            }
        }
        // Phase 2: title_hyper slides from left
        else if (hyperX < (RES_X - bmp.title_hyper.getWidth()) / 2)
        {
            hyperX += 15;
            if (hyperX >= (RES_X - bmp.title_hyper.getWidth()) / 2)
            {
                hyperX = (RES_X - bmp.title_hyper.getWidth()) / 2;
            }
        }
        // Phase 3: title_bg fades in
        else if (bgAlpha < 255)
        {
            bgAlpha += 10;
            if (bgAlpha > 255) bgAlpha = 255;
        }
        // Phase 4: balls fade in
        else if (ballAlpha < 255)
        {
            ballAlpha += 10;
            if (ballAlpha >= 255)
            {
                ballAlpha = 255;
                animComplete = true;
            }
        }
    }
    
    // Enable menu interaction only after animations complete
    if (animComplete)
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
                    case 0: return new SelectPlayer();
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