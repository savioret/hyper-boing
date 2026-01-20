#pragma once

#include "app.h"
#include "sprite.h"


/**
 * MenuBitmaps struct
 * Contains sprites for the main menu.
 */
struct MenuBitmaps
{
    Sprite title_boing;  // Front layer - drops from top
    Sprite title_hyper;  // Middle layer - slides from left
    Sprite title_bg;     // Back layer - fades in
};

/**
 * Menu class
 * Main menu module of the game.
 * Inherits from GameState.
 *
 * Child class of GameState; it is a game module.
 * Represents the main menu of the game.
 */
class Menu : public GameState
{
private:
    MenuBitmaps bmp;
    int xPos, yPos; // legacy position variables
    
    // Animation state for layered title
    int boingY;        // title_boing drops from top
    int hyperX;        // title_hyper slides from left
    int bgAlpha;       // title_bg fades in (0-255)
    bool animComplete; // true when all animations finished
    
    int selectedOption; // 0=PLAY, 1=CONFIGURATION, 2=EXIT
    bool visible; // blinking of the selected option
    int blinkCounter;
    
    // Input state tracking (previously static in moveAll)
    bool upPressed;
    bool downPressed;
    bool enterPressed;

public:
    Menu();
    virtual ~Menu() {}

    int init() override;
    int initBitmaps();
    void drawTitle();
    void drawTitleLayers();
    void drawMenu();

    GameState* moveAll() override;
    int drawAll() override;
    void drawDebugOverlay() override;
    int release() override;

    // Getters for possible access
    int getSelectedOption() const { return selectedOption; }
};