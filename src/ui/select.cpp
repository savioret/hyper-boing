#include "main.h"
#include "appdata.h"
#include "appconsole.h"
#include <SDL.h>

int SelectPlayer::initBitmaps()
{
    bmp.back.init(&appGraph, "graph/titleback.png", 0, 0);
    appGraph.setColorKey(bmp.back.getBmp(), 0xFF0000);

    bmp.mode.init(&appGraph, "graph/selecmodo.png", 0, 0);
    appGraph.setColorKey(bmp.mode.getBmp(), 0xFF0000);
    
    bmp.select[PLAYER1].init(&appGraph, "graph/select1p.png", 0, 0);
    appGraph.setColorKey(bmp.select[PLAYER1].getBmp(), 0x00FF00);
    bmp.select[PLAYER2].init(&appGraph, "graph/select2p.png", 0, 0);
    appGraph.setColorKey(bmp.select[PLAYER2].getBmp(), 0x969696);

    return 1;
}

SelectPlayer::SelectPlayer()
    : xb(0), yb(0), option(0), delay(13), delayCounter(0), initDelay(40)
{
}

int SelectPlayer::init()
{
    gameinf.isMenu() = false;
    initBitmaps();
    xb = yb = 0;
    yb = bmp.back.getHeight();

    option = 0;
    initDelay = 40;

    setGameSpeed(GLOBAL_GAMESPEED);
    difTime1 = 0; 
    difTime2 = gameSpeed;
    time1 = SDL_GetTicks() + gameSpeed;
    time2 = SDL_GetTicks();

    delay = 13;
    delayCounter = 0;

    pause = false;	

    return 1;
}

int SelectPlayer::release()
{
    bmp.select[PLAYER1].release();
    bmp.select[PLAYER2].release();
    // bmp.selText[PLAYER1].release();
    // bmp.selText[PLAYER2].release();
    bmp.mode.release();
    bmp.back.release();

    CloseMusic();

    return 1;
}

void SelectPlayer::drawSelect()
{
    SDL_SetRenderDrawColor(appGraph.getRenderer(), 255, 255, 255, 255);
    if ( option == 0 )
        appGraph.filledRectangle(65, 205, 70 + bmp.select[PLAYER1].getWidth() + 5, 210 + bmp.select[PLAYER1].getHeight() + 5);
    else
        appGraph.filledRectangle(330, 205, 335 + bmp.select[PLAYER2].getWidth() + 5, 210 + bmp.select[PLAYER2].getHeight() + 5);

    appGraph.draw(&bmp.mode, 38, 10);
    appGraph.draw(&bmp.select[PLAYER1], 70, 210);
    appGraph.draw(&bmp.select[PLAYER2], 335, 210);
}

int SelectPlayer::drawAll()
{
    GameState::drawScrollingBackground();
    drawSelect();
    
    // Finalize render (debug overlay, console, flip)
    finalizeRender();
    
    return 1;
}

GameState* SelectPlayer::moveAll()
{
    if (xb < bmp.back.getWidth()) xb++;
    else xb = 0;

    if (yb > 0) yb--;
    else yb = bmp.back.getHeight();

    if (initDelay > 0) initDelay--;

    GameState::updateScrollingBackground();

    if (appInput.key(SDL_SCANCODE_ESCAPE))
        return new Menu();

    if (appInput.key(gameinf.getKeys()[PLAYER1].left) || appInput.key(gameinf.getKeys()[PLAYER1].right))
    {
        if (!delayCounter)
        {
            option = !option;
            delayCounter = delay;
        }
    }
    else if (!initDelay)
    {
        if (appInput.key(SDL_SCANCODE_RETURN) || appInput.key(gameinf.getKeys()[PLAYER1].shoot))
        {
            gameinf.player[PLAYER1] = std::make_unique<Player>(PLAYER1);
            if (option == PLAYER2)
                gameinf.player[PLAYER2] = std::make_unique<Player>(PLAYER2);
            gameinf.initStages();
            return new Scene(&gameinf.getStages()[0]);
        }
    }

    if (delayCounter > 0) delayCounter--;	

    return nullptr;
}