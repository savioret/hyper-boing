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

    // Initialize animation positions (off-screen)
    xt1 = -bmp.title1.getWidth();
    xt2 = 640;
    yt1 = 50;
    yt2 = 50 + bmp.title1.getHeight() + gameinf.getStageRes().fontnum[FONT_HUGE].getHeight() + 25;
    yr1 = -16;
    yr2 = 480;

    xnum = 275;
    ynum = -90;

    // Initialize score displays
    cscore[PLAYER1] = 0;
    cscore[PLAYER2] = 0;

    // Start in TextSlideIn state
    setSubState(LevelClearSubState::TextSlideIn);

    return 1;
}

void StageClear::setSubState(LevelClearSubState newState)
{
    if (currentSubState != newState)
    {
        currentSubState = newState;
        LOG_INFO("StageClear: Entering substate: %d", static_cast<int>(newState));
    }
}

void StageClear::drawAll()
{
    int i, j;
    char cad[10];
    StageResources& res = gameinf.getStageRes();

    std::snprintf(cad, sizeof(cad), "%2d", gameinf.getCurrentStage());
    for (i = 0; i < (int)std::strlen(cad); i++)
        if (cad[i] == ' ') cad[i] = '0';

    // Draw red brick curtains during closing/opening phases
    if (currentSubState == LevelClearSubState::CurtainClosing ||
        currentSubState == LevelClearSubState::TextSlideOut)
    {
        // Curtain closing animation (from edges toward center)
        for (i = -16; i < (yr1 / 16) + 1; i++)
            for (j = 0; j < 40; j++)
            {
                appGraph.draw(&bmp.roof, j * 16, i * 16 - (i % 16));
                appGraph.draw(&bmp.roof, j * 16, 480 - (i * 16));
            }
    }
    else if (currentSubState == LevelClearSubState::CurtainOpening)
    {
        // Curtain opening animation (from center toward edges)
        for (i = (yr1 / 16) + 1; i > -17; i--)
            for (j = 0; j < 40; j++)
            {
                appGraph.draw(&bmp.roof, j * 16, i * 16 - (i % 16));
                appGraph.draw(&bmp.roof, j * 16, 480 - (i * 16));
            }
    }

    // Always draw title text and stage number
    appGraph.draw(&bmp.title1, xt1, yt1);
    appGraph.draw(&bmp.title2, xt2, yt2);
    appGraph.draw(&scene->fontNum[FONT_HUGE], cad, xnum, ynum);

    // Don't draw player icons/scores during text slide out or curtain opening
    if (currentSubState == LevelClearSubState::TextSlideOut ||
        currentSubState == LevelClearSubState::CurtainOpening)
        return;

    // Draw player icons (but not during curtain closing)
    if (currentSubState != LevelClearSubState::CurtainClosing &&
        currentSubState != LevelClearSubState::TextSlideOut)
    {
        if (gameinf.getPlayer(PLAYER1)->isPlaying())
        {
            appGraph.draw(&res.miniplayer[PLAYER1], 40, 300);
        }
        if (gameinf.getPlayer(PLAYER2))
            if (gameinf.getPlayer(PLAYER2)->isPlaying())
            {
                appGraph.draw(&res.miniplayer[PLAYER2], 350, 300);
            }
    }

    // Draw scores after text has slid in
    if (currentSubState != LevelClearSubState::TextSlideIn &&
        currentSubState != LevelClearSubState::CurtainClosing &&
        currentSubState != LevelClearSubState::TextSlideOut)
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
 * This function manages the stage clear sequence using a state machine.
 * The sequence progresses through: TextSlideIn -> ScoreCounting -> WaitingForInput ->
 * CurtainClosing -> TextSlideOut -> CurtainOpening
 *
 * @return 1 = continue (stay in Scene), 0 = ready screen complete, -1 = transition to next stage
 */
int StageClear::moveAll()
{
    bool a = false, b = false, c = false;

    switch (currentSubState)
    {
        case LevelClearSubState::TextSlideIn:
        {
            // Slide "LEVEL COMPLETED" text into position
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
                setSubState(LevelClearSubState::ScoreCounting);
            }
            break;
        }

        case LevelClearSubState::ScoreCounting:
        {
            // Check if player wants to skip counting
            bool skipRequested = false;
            if (appInput.key(gameinf.getKeys()[PLAYER1].shoot))
                skipRequested = true;
            else if (gameinf.getPlayer(PLAYER2))
                if (appInput.key(gameinf.getKeys()[PLAYER2].shoot))
                    skipRequested = true;

            if (skipRequested)
            {
                // Instantly finish score counting
                cscore[PLAYER1] = gameinf.getPlayer(PLAYER1)->getScore();
                if (gameinf.getPlayer(PLAYER2))
                    cscore[PLAYER2] = gameinf.getPlayer(PLAYER2)->getScore();
                LOG_INFO("StageClear: Player skipped score counting");
                setSubState(LevelClearSubState::WaitingForInput);
                break;
            }

            // Increment scores gradually
            bool player1Done = false;
            bool player2Done = true;  // Assume done if no player 2

            if (cscore[PLAYER1] < gameinf.getPlayer(PLAYER1)->getScore())
                cscore[PLAYER1]++;
            else
                player1Done = true;

            if (gameinf.getPlayer(PLAYER2))
            {
                if (cscore[PLAYER2] < gameinf.getPlayer(PLAYER2)->getScore())
                {
                    cscore[PLAYER2]++;
                    player2Done = false;
                }
                else
                    player2Done = true;
            }

            // Check if both players finished counting
            if (player1Done && player2Done)
            {
                setSubState(LevelClearSubState::WaitingForInput);
            }
            break;
        }

        case LevelClearSubState::WaitingForInput:
        {
            // Wait for player to press fire button to continue
            if (appInput.key(gameinf.getKeys()[PLAYER1].shoot))
            {
                LOG_INFO("StageClear: Player 1 pressed fire, starting curtain close");
                setSubState(LevelClearSubState::CurtainClosing);
            }
            else if (gameinf.getPlayer(PLAYER2))
            {
                if (appInput.key(gameinf.getKeys()[PLAYER2].shoot))
                {
                    LOG_INFO("StageClear: Player 2 pressed fire, starting curtain close");
                    setSubState(LevelClearSubState::CurtainClosing);
                }
            }
            break;
        }

        case LevelClearSubState::CurtainClosing:
        {
            // Close red brick curtain from top and bottom
            if (yr1 < 240) yr1 += 4;
            if (yr2 > 241) yr2 -= 4;
            else
            {
                setSubState(LevelClearSubState::TextSlideOut);
            }
            break;
        }

        case LevelClearSubState::TextSlideOut:
        {
            // Slide text off screen
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
                setSubState(LevelClearSubState::CurtainOpening);
                return -1;  // Signal to Scene: transition to next stage
            }
            break;
        }

        case LevelClearSubState::CurtainOpening:
        {
            // Open red brick curtain
            if (yr1 > -32) yr1 -= 4;
            if (yr2 < 481) yr2 += 4;
            else
            {
                LOG_INFO("StageClear: Curtain opened, ready screen can start");
                return 0;  // Signal to Scene: ready screen should start
            }
            break;
        }
    }

    return 1;  // Continue in current state
}

int StageClear::release()
{
    bmp.title1.release();
    bmp.title2.release();
    return 1;
}
