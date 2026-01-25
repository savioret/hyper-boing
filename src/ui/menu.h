#pragma once

#include "app.h"
#include "sprite.h"
#include "action.h"
#include "menutitle.h"
#include <memory>

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
    // Title animation (encapsulated in MenuTitle)
    std::unique_ptr<MenuTitle> menuTitle;

    int selectedOption; // 0=PLAY, 1=CONFIGURATION, 2=EXIT
    bool visible; // blinking of the selected option
    std::unique_ptr<Action> blinkAction;

    // Input state tracking (previously static in moveAll)
    bool upPressed;
    bool downPressed;
    bool enterPressed;

public:
    Menu();
    virtual ~Menu() {}

    int init() override;
    int initBitmaps();
    void drawMenu();

    GameState* moveAll(float dt) override;
    int drawAll() override;
    void drawDebugOverlay() override;
    int release() override;

    // Getters for possible access
    int getSelectedOption() const { return selectedOption; }
};