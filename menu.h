#pragma once

#include "app.h"
#include "sprite.h"


/**
 * MenuBitmaps struct
 * Contains sprites for the main menu.
 */
struct MenuBitmaps
{
    Sprite title;
    Sprite menuFont; // BMFont texture
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
    int xPos, yPos; // title position
    int selectedOption; // 0=PLAY, 1=CONFIGURATION, 2=EXIT
    bool visible; // blinking of the selected option
    int blinkCounter;

public:
    Menu();
    virtual ~Menu() {}

    int init() override;
    int initBitmaps();
    void drawTitle();
    void drawMenu();

    void* moveAll() override;
    int drawAll() override;
    void drawDebugOverlay() override;
    int release() override;

    // Getters for possible access
    int getSelectedOption() const { return selectedOption; }
};