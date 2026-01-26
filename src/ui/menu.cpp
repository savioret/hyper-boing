#include "main.h"
#include "appdata.h"
#include "appconsole.h"
#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include <cstring>

int Menu::initBitmaps()
{
    // Initialize shared background
    GameState::initSharedBackground();

    // TODO: Load fonts in GameState or AppData
    fontRenderer.loadFont(&appGraph, "assets/fonts/thickfont_grad_64.fnt");

    return 1;
}

Menu::Menu()
    : selectedOption(0), visible(true),
      upPressed(false), downPressed(false), enterPressed(false)
{
}

int Menu::init()
{
    GameState::init();

    gameinf.isMenu() = true;
    initBitmaps();

    selectedOption = 0; // PLAY

    // Create and initialize MenuTitle
    menuTitle = std::make_unique<MenuTitle>(&appGraph);
    menuTitle->init();

    // Create blink action for menu option (0.5 second intervals, infinite)
    blinkAction = blink(&visible, 0.5f, 0);  // 0 = infinite

    CloseMusic();
    OpenMusic("assets/music/menu.ogg");
    PlayMusic();

    return 1;
}

int Menu::release()
{
    menuTitle.reset();  // MenuTitle destructor handles sprite cleanup
    fontRenderer.release();  // BMFontRenderer handles its own cleanup

    CloseMusic();

    return 1;
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

    if (menuTitle)
    {
        menuTitle->draw();
    }

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

    textOverlay.addTextF("AnimComplete=%s Selected=%d",
        (menuTitle && menuTitle->isAnimationFinished()) ? "YES" : "NO", selectedOption);

    textOverlay.addTextF("Scroll X=%d Y=%d", (int)appData.scrollX, (int)appData.scrollY);
}

GameState* Menu::moveAll(float dt)
{
    // Update blink animation
    if (blinkAction)
    {
        blinkAction->update(dt);
    }

    GameState::updateScrollingBackground();

    // Update title animation
    if (menuTitle)
    {
        menuTitle->update(dt);
    }

    // Enable menu interaction only after animations complete
    if (menuTitle && menuTitle->isAnimationFinished())
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