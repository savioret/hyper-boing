#include <cstdio>
#include <cstring>
#include "pang.h"
#include "appdata.h"

StageClear::StageClear(Scene* scn, int targetStageNum)
    : targetStage(targetStageNum)
{
    scene = scn;
    init();
}

StageClear::~StageClear()
{
    release();
}

int StageClear::init()
{
    bmp.title1.init(&appGraph, "graph/nivel.png", 0, 0);
    appGraph.setColorKey(bmp.title1.getBmp(), 0x00FF00);
    bmp.title2.init(&appGraph, "graph/completado.png", 0, 0);
    appGraph.setColorKey(bmp.title2.getBmp(), 0x00FF00);
    bmp.roof.init(&appGraph, "graph/ladrill4.png", 0, 0);
    appGraph.setColorKey(bmp.roof.getBmp(), 0x00FF00);

    xt1 = -bmp.title1.getWidth();
    xt2 = 640;
    yt1 = 50;
    yt2 = 50 + bmp.title1.getHeight() + scene->bmp.fontnum[FONT_HUGE].getHeight() + 25;
    yr1 = -16;
    yr2 = 480;

    xnum = 275;
    ynum = -90;

    cscore[PLAYER1] = 0;
    cscore[PLAYER2] = 0;

    endMove = false;
    endCount = false;
    movingOut = false;
    finish = false;
    isClosing = isOpening = false;
    endClose = endOpening = false;

    return 1;
}

void StageClear::drawAll()
{
    int i, j;
    char cad[10];
    
    std::snprintf(cad, sizeof(cad), "%2d", gameinf.getCurrentStage());
    for (i = 0; i < (int)std::strlen(cad); i++)
        if (cad[i] == ' ') cad[i] = '0';

    if (isClosing)
    {			
        for (i = -16; i < (yr1 / 16) + 1; i++)
            for (j = 0; j < 40; j++)
            {	
                appGraph.drawClipped(&bmp.roof, j * 16, i * 16 - (i % 16));
                appGraph.drawClipped(&bmp.roof, j * 16, 480 - (i * 16));
            }
    }	
    else if (isOpening)
    {			
        for (i = (yr1 / 16) + 1; i > -17; i--)
            for (j = 0; j < 40; j++)
            {	
                appGraph.drawClipped(&bmp.roof, j * 16, i * 16 - (i % 16));
                appGraph.drawClipped(&bmp.roof, j * 16, 480 - (i * 16));
            }
    }	

    appGraph.drawClipped(&bmp.title1, xt1, yt1);
    appGraph.drawClipped(&bmp.title2, xt2, yt2);
    appGraph.drawClipped(&scene->fontNum[FONT_HUGE], cad, xnum, ynum);
    
    if (finish) return;
    if (!isClosing)
    {
        if (gameinf.getPlayers()[PLAYER1]->isPlaying())
            appGraph.draw(&scene->bmp.miniplayer[PLAYER1], 40, 300);
        if (gameinf.getPlayers()[PLAYER2])
            if (gameinf.getPlayers()[PLAYER2]->isPlaying())
                appGraph.draw(&scene->bmp.miniplayer[PLAYER2], 350, 300);
    }

    if (endMove && !isClosing)
    {
        if (gameinf.getPlayers()[PLAYER1]->isPlaying())
            appGraph.draw(&scene->fontNum[FONT_SMALL], cscore[PLAYER1], 105, 320);
        if (gameinf.getPlayers()[PLAYER2])
            if (gameinf.getPlayers()[PLAYER2]->isPlaying())
                appGraph.draw(&scene->fontNum[FONT_SMALL], cscore[PLAYER2], 450, 320);
    }
}

/**
 * StageClear logic
 *
 * This function manages the stage clear sequence: moving text into screen,
 * incrementing scores, and waiting for player appInput to proceed.
 */
int StageClear::moveAll()
{
    bool a = false, b = false, c = false;

    if (appInput.key(gameinf.getKeys()[PLAYER1].shoot))
    {
        if (!endCount)
        {
            endCount = true;
            cscore[PLAYER1] = gameinf.getPlayers()[PLAYER1]->getScore();
            if (gameinf.getPlayers()[PLAYER2])
                cscore[PLAYER2] = gameinf.getPlayers()[PLAYER2]->getScore();
        }
        isClosing = true;
    }
    else if (gameinf.getPlayers()[PLAYER2])
    {
        if (appInput.key(gameinf.getKeys()[PLAYER2].shoot))
        {
            if (!endCount)
            {
                endCount = true;				
                cscore[PLAYER1] = gameinf.getPlayers()[PLAYER1]->getScore();
                if (gameinf.getPlayers()[PLAYER2])
                    cscore[PLAYER2] = gameinf.getPlayers()[PLAYER2]->getScore();
            }
            isClosing = true;
        }
    }

    if (isClosing)
    {
        if (yr1 < 240) yr1 += 4;
        if (yr2 > 241) yr2 -= 4;
        else 
        {
            endClose = true;
            movingOut = true;
        }
    }
    else if (isOpening)
    {
        if (yr1 > -32) yr1 -= 4;
        if (yr2 < 481) yr2 += 4;
        else 
        {
            endOpening = true;
            isOpening = false;
            movingOut = true;
            return 0;
        }
    }

    if (movingOut)
    {
        if (xt1 < 640)
            xt1 += 4;
        else a = true;

        if (xt2 > -bmp.title2.getWidth())
            xt2 -= 5;
        else b = true;

        if (ynum < 480)
            ynum += 5;
        else c = true;

        if (a && b && c)
        {
            finish = true;
            isOpening = true;
            isClosing = false;
            movingOut = false;
            return -1;
        }
    }

    if (!endMove)
    {
        if (xt1 < 250)
            xt1 += 4;
        else a = true;

        if (xt2 > 135)
            xt2 -= 5;
        else b = true;

        if (ynum < 100)
            ynum += 3;
        else c = true;

        if (a && b && c) endMove = true;
    }
    else if (!endCount)
    {
        cscore[PLAYER1]++;			

        if (gameinf.getPlayers()[PLAYER2])
        {
            if (cscore[PLAYER2] < gameinf.getPlayers()[PLAYER2]->getScore())
                cscore[PLAYER2]++;
            if (cscore[PLAYER1] >= gameinf.getPlayers()[PLAYER1]->getScore() &&
                cscore[PLAYER2] >= gameinf.getPlayers()[PLAYER2]->getScore())
            {
                endCount = true;
                isClosing = true;
            }
        }
        else if (cscore[PLAYER1] >= gameinf.getPlayers()[PLAYER1]->getScore())
        {
            endCount = true;
            isClosing = true;
        }
    }

    return 1;
}

int StageClear::release()
{
    bmp.title1.release();
    bmp.title2.release();
    return 1;
}
