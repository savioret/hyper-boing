#include <cstdio>
#include <cstring>
#include "../main.h"
#include "appdata.h"
#include "logger.h"

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
    bmp.title1.init(&appGraph, "assets/graph/ui/nivel.png", 0, 0);
    appGraph.setColorKey(bmp.title1.getBmp(), 0x00FF00);
    bmp.title2.init(&appGraph, "assets/graph/ui/completado.png", 0, 0);
    appGraph.setColorKey(bmp.title2.getBmp(), 0x00FF00);
    bmp.roof.init(&appGraph, "assets/graph/entities/ladrill4.png", 0, 0);
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
                appGraph.draw(&bmp.roof, j * 16, i * 16 - (i % 16));
                appGraph.draw(&bmp.roof, j * 16, 480 - (i * 16));
            }
    }
    else if (isOpening)
    {
        for (i = (yr1 / 16) + 1; i > -17; i--)
            for (j = 0; j < 40; j++)
            {
                appGraph.draw(&bmp.roof, j * 16, i * 16 - (i % 16));
                appGraph.draw(&bmp.roof, j * 16, 480 - (i * 16));
            }
    }

    // Set sprite positions before drawing
    // bmp.title1.setPos(xt1, yt1);
    // bmp.title2.setPos(xt2, yt2);

    appGraph.draw(&bmp.title1, xt1, yt1);
    appGraph.draw(&bmp.title2, xt2, yt2);
    appGraph.draw(&scene->fontNum[FONT_HUGE], cad, xnum, ynum);

    if (finish) return;
    if (!isClosing)
    {
        if (gameinf.getPlayer(PLAYER1)->isPlaying())
        {
            appGraph.draw(&scene->bmp.miniplayer[PLAYER1], 40, 300);
        }
        if (gameinf.getPlayer(PLAYER2))
            if (gameinf.getPlayer(PLAYER2)->isPlaying())
            {
                appGraph.draw(&scene->bmp.miniplayer[PLAYER2], 350, 300);
            }
    }

    if (endMove && !isClosing)
    {
        if (gameinf.getPlayer(PLAYER1)->isPlaying())
            appGraph.draw(&scene->fontNum[FONT_SMALL], cscore[PLAYER1], 105, 320);
        if (gameinf.getPlayer(PLAYER2))
            if (gameinf.getPlayer(PLAYER2)->isPlaying())
                appGraph.draw(&scene->fontNum[FONT_SMALL], cscore[PLAYER2], 450, 320);
    }
}

/**
 * StageClear logic
 *
 * This function manages the stage clear sequence: moving text into screen,
 * incrementing scores, and waiting for player input to proceed.
 */
int StageClear::moveAll()
{
    bool a = false, b = false, c = false;

    // Only allow progression to closing when score counting is complete
    if (endCount && !isClosing)
    {
        if (appInput.key(gameinf.getKeys()[PLAYER1].shoot))
        {
            LOG_INFO("StageClear: Player 1 pressed fire, starting transition");
            isClosing = true;
        }
        else if (gameinf.getPlayer(PLAYER2))
        {
            if (appInput.key(gameinf.getKeys()[PLAYER2].shoot))
            {
                LOG_INFO("StageClear: Player 2 pressed fire, starting transition");
                isClosing = true;
            }
        }
    }

    if (isClosing)
    {
        if (yr1 < 240) yr1 += 4;
        if (yr2 > 241) yr2 -= 4;
        else 
        {
            LOG_INFO("StageClear: Red blocks closed, moving text out");
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
            LOG_INFO("StageClear: Red blocks opened, ready screen can start");
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
            LOG_INFO("StageClear: Text moved out, starting red blocks opening");
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

        if (a && b && c) 
        {
            endMove = true;
            LOG_INFO("StageClear: Text in position, starting score count");
        }
    }
    else if (!endCount)
    {
        cscore[PLAYER1]++;			

        if (gameinf.getPlayer(PLAYER2))
        {
            if (cscore[PLAYER2] < gameinf.getPlayer(PLAYER2)->getScore())
                cscore[PLAYER2]++;
            if (cscore[PLAYER1] >= gameinf.getPlayer(PLAYER1)->getScore() &&
                cscore[PLAYER2] >= gameinf.getPlayer(PLAYER2)->getScore())
            {
                endCount = true;
                LOG_INFO("StageClear: Score counting complete, waiting for button press");
                // Don't automatically close - wait for button press
            }
        }
        else if (cscore[PLAYER1] >= gameinf.getPlayer(PLAYER1)->getScore())
        {
            endCount = true;
            LOG_INFO("StageClear: Score counting complete, waiting for button press");
            // Don't automatically close - wait for button press
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
