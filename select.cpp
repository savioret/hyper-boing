#include "pang.h"
#include <SDL.h>

int SelectSync::initBitmaps()
{
    bmp.back.init(&graph, "graph\\titleback.png", 0, 0);
    graph.setColorKey(bmp.back.getBmp(), 0xFF0000);

    bmp.mode.init(&graph, "graph\\selecmodo.png", 0, 0);
    graph.setColorKey(bmp.mode.getBmp(), 0xFF0000);
    
    // bmp.selText[PLAYER1].init(&graph, "graph\\select1ptext.png", 0, 0);
    // graph.setColorKey(bmp.selText[PLAYER1].getBmp(), 0x0000FF);
    // bmp.selText[PLAYER2].init(&graph, "graph\\select2ptext.png", 0, 0);
    // graph.setColorKey(bmp.selText[PLAYER2].getBmp(), 0x0000FF);

    bmp.select[PLAYER1].init(&graph, "graph\\select1p.png", 0, 0);
    graph.setColorKey(bmp.select[PLAYER1].getBmp(), 0x00FF00);
    bmp.select[PLAYER2].init(&graph, "graph\\select2p.png", 0, 0);
    graph.setColorKey(bmp.select[PLAYER2].getBmp(), 0x969696);

    return 1;
}

SelectSync::SelectSync()
    : xb(0), yb(0), option(0), delay(13), delayCounter(0), initDelay(40)
{
}

int SelectSync::init()
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

int SelectSync::release()
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

void SelectSync::drawSelect()
{
    SDL_SetRenderDrawColor(graph.getRenderer(), 255, 255, 255, 255);
    if ( option == 0 )
        graph.filledRectangle(65, 205, 70 + bmp.select[PLAYER1].getWidth() + 5, 210 + bmp.select[PLAYER1].getHeight() + 5);
    else
        graph.filledRectangle(330, 205, 335 + bmp.select[PLAYER2].getWidth() + 5, 210 + bmp.select[PLAYER2].getHeight() + 5);

    graph.draw(&bmp.mode, 38, 10);
    graph.draw(&bmp.select[PLAYER1], 70, 210);
    //graph.draw(&bmp.selText[PLAYER1], 70, 210 + bmp.select[PLAYER1].getHeight() + 10);
    graph.draw(&bmp.select[PLAYER2], 335, 210);
    //graph.draw(&bmp.selText[PLAYER2], 350, 210 + bmp.select[PLAYER2].getHeight() + 10);
}


int SelectSync::drawAll()
{
    App::drawScrollingBackground();
    drawSelect();
    graph.flip();
    return 1;
}

void* SelectSync::moveAll()
{
    if (xb < bmp.back.getWidth()) xb++;
    else xb = 0;

    if (yb > 0) yb--;
    else yb = bmp.back.getHeight();

    if (initDelay > 0) initDelay--;

    App::updateScrollingBackground();

    if (input.key(SDL_SCANCODE_ESCAPE))
        return new Menu();

    if (input.key(gameinf.getKeys()[PLAYER1].left) || input.key(gameinf.getKeys()[PLAYER1].right))
    {
        if (!delayCounter)
        {
            option = !option;
            delayCounter = delay;
        }
    }
    else if (!initDelay)
    {
        if (input.key(SDL_SCANCODE_RETURN) || input.key(gameinf.getKeys()[PLAYER1].shoot))
        {
            gameinf.getPlayers()[PLAYER1] = new Player(PLAYER1);
            if (option == PLAYER2)
                gameinf.getPlayers()[PLAYER2] = new Player(PLAYER2);
            gameinf.initStages();
            return new Scene(&gameinf.getStages()[0]);
        }
    }

    if (delayCounter > 0) delayCounter--;	

    return nullptr;
}