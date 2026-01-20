#pragma once

#include "app.h"
#include "sprite.h"

/**
 * SelectBitmaps struct
 * Contains sprites for the player selection screen.
 */
struct SelectBitmaps
{
    Sprite back;
    Sprite select[2];
    Sprite selText[2];
    Sprite mode;
};

/**
 * SelectPlayer class
 *
 * This module handles the player selection screen (1 Player or 2 Players).
 * It manages the visual selection feedback and transitioning to the game.
 * Inherits from GameState.
 */
class SelectPlayer : public GameState
{
private:
    SelectBitmaps bmp;
    int xb, yb; // background x and y
    int option;

    int delay; // time between option changes
    int delayCounter;
    int initDelay; // to prevent immediate selection

public:
    SelectPlayer();
    virtual ~SelectPlayer() {}

    int init() override;
    int initBitmaps();
    void drawSelect();
    void drawBack();	

    GameState* moveAll() override;
    int drawAll() override;
    int release() override;
};