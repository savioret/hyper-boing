#include "appdata.h"
#include "app.h"
#include "sprite.h"
#include "player.h"
#include "stage.h"
#include "main.h"
#include <cstdlib>

// Temporarily undefine macros that conflict with member names during construction
#ifdef quit
#undef quit
#endif
#ifdef goBack  
#undef goBack
#endif
#ifdef globalmode
#undef globalmode
#endif

// Initialize static singleton instance
std::unique_ptr<AppData> AppData::s_instance = nullptr;

AppData::AppData()
    : numPlayers(1), numStages(6), currentStage(1), inMenu(true),
      activeScene(nullptr), sharedBackground(nullptr), scrollX(0.0f), 
      scrollY(0.0f), backgroundInitialized(false), debugMode(false),
      quit(false), goBack(false), renderMode(RENDERMODE_NORMAL),
      currentScreen(nullptr), nextScreen(nullptr)
{
    player[PLAYER1] = nullptr;
    player[PLAYER2] = nullptr;
    
    // Initialize stages vector
    stages.resize(6);
}

// Redefine macros after construction
#define quit AppData::instance().quit
#define goback AppData::instance().goBack
#define globalmode AppData::instance().renderMode

AppData& AppData::instance()
{
    if (!s_instance)
    {
        s_instance = std::make_unique<AppData>();
    }
    return *s_instance;
}

void AppData::destroy()
{
    if (s_instance)
    {
        s_instance->release();
        s_instance.reset();
    }
}

void AppData::init()
{
    inMenu = true;

    // Initialize Player 1 sprites
    bitmaps.player[PLAYER1][0].init(&appGraph, "graph/p1k1l.png", 0, 3);
    bitmaps.player[PLAYER1][1].init(&appGraph, "graph/p1k2l.png", 4, 3);
    bitmaps.player[PLAYER1][2].init(&appGraph, "graph/p1k3l.png", 6, 3);
    bitmaps.player[PLAYER1][3].init(&appGraph, "graph/p1k4l.png", 4, 3);
    bitmaps.player[PLAYER1][4].init(&appGraph, "graph/p1k5l.png", 4, 3);
    
    bitmaps.player[PLAYER1][5].init(&appGraph, "graph/p1k1r.png", 0, 3);
    bitmaps.player[PLAYER1][6].init(&appGraph, "graph/p1k2r.png", 0, 3);
    bitmaps.player[PLAYER1][7].init(&appGraph, "graph/p1k3r.png", 0, 3);
    bitmaps.player[PLAYER1][8].init(&appGraph, "graph/p1k4r.png", 0, 3);
    bitmaps.player[PLAYER1][9].init(&appGraph, "graph/p1k5r.png", 0, 3);

    bitmaps.player[PLAYER1][10].init(&appGraph, "graph/p1shoot1.png", 13, 0);
    bitmaps.player[PLAYER1][11].init(&appGraph, "graph/p1shoot2.png", 13, 3);
    bitmaps.player[PLAYER1][12].init(&appGraph, "graph/p1win.png", 13, 4);
    bitmaps.player[PLAYER1][13].init(&appGraph, "graph/p1dead.png");
    bitmaps.player[PLAYER1][14].init(&appGraph, "graph/p1dead2.png");
    bitmaps.player[PLAYER1][15].init(&appGraph, "graph/p1dead3.png");
    bitmaps.player[PLAYER1][16].init(&appGraph, "graph/p1dead4.png");
    bitmaps.player[PLAYER1][17].init(&appGraph, "graph/p1dead5.png");
    bitmaps.player[PLAYER1][18].init(&appGraph, "graph/p1dead6.png");
    bitmaps.player[PLAYER1][19].init(&appGraph, "graph/p1dead7.png");
    bitmaps.player[PLAYER1][20].init(&appGraph, "graph/p1dead8.png");

    for (int i = 0; i < 21; i++)
        appGraph.setColorKey(bitmaps.player[PLAYER1][i].getBmp(), 0x00FF00);

    // Initialize Player 2 sprites
    bitmaps.player[PLAYER2][0].init(&appGraph, "graph/p2k1l.png", 0, 3);
    bitmaps.player[PLAYER2][1].init(&appGraph, "graph/p2k2l.png", 4, 3);
    bitmaps.player[PLAYER2][2].init(&appGraph, "graph/p2k3l.png", 6, 3);
    bitmaps.player[PLAYER2][3].init(&appGraph, "graph/p2k4l.png", 4, 3);
    bitmaps.player[PLAYER2][4].init(&appGraph, "graph/p2k5l.png", 4, 3);
    
    bitmaps.player[PLAYER2][5].init(&appGraph, "graph/p2k1r.png", 0, 3);
    bitmaps.player[PLAYER2][6].init(&appGraph, "graph/p2k2r.png", 0, 3);
    bitmaps.player[PLAYER2][7].init(&appGraph, "graph/p2k3r.png", 0, 3);
    bitmaps.player[PLAYER2][8].init(&appGraph, "graph/p2k4r.png", 0, 3);
    bitmaps.player[PLAYER2][9].init(&appGraph, "graph/p2k5r.png", 0, 3);

    bitmaps.player[PLAYER2][10].init(&appGraph, "graph/p2shoot1.png", 13, 0);
    bitmaps.player[PLAYER2][11].init(&appGraph, "graph/p2shoot2.png", 13, 3);
    bitmaps.player[PLAYER2][12].init(&appGraph, "graph/p2win.png", 13, 4);
    bitmaps.player[PLAYER2][13].init(&appGraph, "graph/p2dead.png");
    bitmaps.player[PLAYER2][14].init(&appGraph, "graph/p2dead2.png");
    bitmaps.player[PLAYER2][15].init(&appGraph, "graph/p2dead3.png");
    bitmaps.player[PLAYER2][16].init(&appGraph, "graph/p2dead4.png");
    bitmaps.player[PLAYER2][17].init(&appGraph, "graph/p2dead5.png");
    bitmaps.player[PLAYER2][18].init(&appGraph, "graph/p2dead6.png");
    bitmaps.player[PLAYER2][19].init(&appGraph, "graph/p2dead7.png");
    bitmaps.player[PLAYER2][20].init(&appGraph, "graph/p2dead8.png");

    for (int i = 0; i < 21; i++)
        appGraph.setColorKey(bitmaps.player[PLAYER2][i].getBmp(), 0xFF0000);

    // Initialize default key bindings
    playerKeys[PLAYER1].set(SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE);
    playerKeys[PLAYER2].set(SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_LCTRL);
}

void AppData::initStages()
{
    numStages = 6;

    for (int i = 0; i < numStages; i++)
        stages[i].reset();

    /*************** START OF SCREENS ***********************/
    /***** USING NEW CLEAN BUILDER API                  *****/

    /* Stage 1 - Clean builder syntax */
    int i = 0;
    stages[i].xpos[PLAYER1] = 250;
    stages[i].xpos[PLAYER2] = 350;
    stages[i].setBack("fondo1.png");
    stages[i].setMusic("stage1.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    
    // Floors using clean builder syntax
    stages[i].spawn(StageObjectBuilder::floor().at(550, 50).type(0).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(250, 250).type(0).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(350, 150).type(1).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(550, 150).type(1).time(0));
    
    // Balls at top with random X
    stages[i].spawn(StageObjectBuilder::ball().time(1).atMaxY());
    stages[i].spawn(StageObjectBuilder::ball().time(20).atMaxY());

    /* Stage 2 - Complex formation */
    i = 1;
    stages[i].xpos[PLAYER1] = stages[i].xpos[PLAYER2] = 270;
    stages[i].setBack("fondo2.png");
    stages[i].setMusic("stage2.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    
    // Create small balls in formation (fixed positions)
    for (int y = 0; y < 2; y++)
    {
        int dirX = (y == 0) ? -1 : 1;
        for (int x = 0; x < 10; x++)
        {
            stages[i].spawn(
                StageObjectBuilder::ball()
                    .at(128 + 300 * y + x * 16, 100)
                    .time(1)
                    .size(3)
                    .top(200)
                    .dir(dirX, 1)
            );
        }
    }

    /* Stage 3 - Mixed approaches */
    i = 2;
    stages[i].xpos[PLAYER1] = 200;
    stages[i].xpos[PLAYER2] = 350;
    stages[i].setBack("fondo3.png");
    stages[i].setMusic("stage3.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    
    // Floor
    stages[i].spawn(StageObjectBuilder::floor().at(250, 70).type(0).time(0));
    
    // Random top balls
    stages[i].spawn(StageObjectBuilder::ball().time(1).atMaxY());
    stages[i].spawn(StageObjectBuilder::ball().time(1).atMaxY());
    
    // Balls with random X at specific Y
    stages[i].spawn(StageObjectBuilder::ball().time(1).size(2).atY(400));
    stages[i].spawn(StageObjectBuilder::ball().time(1).size(2).atY(400).dir(-1, 1));

    /* Stage 4 - Now using new API */
    i = 3;
    stages[i].setBack("fondo4.png");
    stages[i].setMusic("stage4.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    
    stages[i].spawn(StageObjectBuilder::floor().at(250, 70).type(0).time(0));
    
    // Small ball at random top position
    stages[i].spawn(StageObjectBuilder::ball().time(1).size(3).atMaxY());
    
    // Regular balls at random top positions
    stages[i].spawn(StageObjectBuilder::ball().time(1).atMaxY());
    stages[i].spawn(StageObjectBuilder::ball().time(20).atMaxY());

    /* Stage 5 - Staircases and side spawns */
    i = 4;
    stages[i].xpos[PLAYER1] = 250;
    stages[i].xpos[PLAYER2] = 350;
    stages[i].setBack("fondo5.png");
    stages[i].setMusic("stage5.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    
    // Left staircase
    stages[i].spawn(StageObjectBuilder::floor().at(16, 100).type(0).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(80, 164).type(0).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(144, 164).type(1).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(144, 228).type(0).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(208, 228).type(1).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(208, 292).type(0).time(0));
    
    // Right staircase
    stages[i].spawn(StageObjectBuilder::floor().at(RES_X - 80, 100).type(0).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(RES_X - 128, 164).type(0).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(RES_X - 144, 164).type(1).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(RES_X - 192, 228).type(0).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(RES_X - 208, 228).type(1).time(0));
    stages[i].spawn(StageObjectBuilder::floor().at(RES_X - 256, 292).type(0).time(0));
    
    // Small balls spawning from sides (fixed X positions)
    for (int x = 0; x < 15; x++)
    {
        int randomTop = std::rand() % 150 + 150;
        
        // Left side
        stages[i].spawn(
            StageObjectBuilder::ball()
                .at(17, 50)
                .time(5 * x)
                .size(3)
                .top(randomTop)
                .dir(1, 1)
        );
        
        // Right side
        stages[i].spawn(
            StageObjectBuilder::ball()
                .at(MAX_X - 30, 50)
                .time(5 * x)
                .size(3)
                .top(randomTop)
                .dir(-1, 1)
        );
    }

    /* Stage 6 - Grid pattern */
    i = 5;
    stages[i].xpos[PLAYER1] = 250;
    stages[i].xpos[PLAYER2] = 350;
    stages[i].setBack("fondo6.png");
    stages[i].setMusic("stage6.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    
    // Floor grid
    for (int x = 56; x < 600; x += 64)
    {
        for (int y = 22; y < 288; y += 64)
        {
            stages[i].spawn(StageObjectBuilder::floor().at(x, y).type(1).time(0));
        }
    }
    
    // Ball grid with gaps for players (all fixed positions)
    for (int x = 10; x < 650; x += 64)
    {
        if (x < 250 || x > 350)
        {
            stages[i].spawn(
                StageObjectBuilder::ball()
                    .at(x, 395)
                    .time(1)
                    .size(1)
                    .top(395)
                    .dir(0, 1)
            );
            
            stages[i].spawn(
                StageObjectBuilder::ball()
                    .at(x, 150)
                    .time(1)
                    .size(1)
                    .top(395)
                    .dir(0, 1)
            );
        }
    }
}

void AppData::setCurrent(GameState* state)
{
    activeScene = state;
}

void AppData::release()
{
    // Release player bitmaps
    for (int i = 0; i < 21; i++)
    {
        bitmaps.player[PLAYER1][i].release();
        bitmaps.player[PLAYER2][i].release();
    }
    
    // Release shared background (unique_ptr will auto-delete, just release SDL resources)
    if (backgroundInitialized && sharedBackground)
    {
        sharedBackground->release();
        sharedBackground.reset();
        backgroundInitialized = false;
    }
    
    // Players are unique_ptrs, will auto-delete
    player[PLAYER1].reset();
    player[PLAYER2].reset();
    
    // Vector will auto-clean up stages
}

void AppData::preloadMenuMusic()
{
    AudioManager::instance().preloadMusic("music/menu.ogg");
}

void AppData::preloadStageMusic()
{
    // Preload all stage music tracks
    AudioManager::instance().preloadMusic("music/stage1.ogg");
    AudioManager::instance().preloadMusic("music/stage2.ogg");
    AudioManager::instance().preloadMusic("music/stage3.ogg");
    AudioManager::instance().preloadMusic("music/stage4.ogg");
    AudioManager::instance().preloadMusic("music/stage5.ogg");
}
