#include "appdata.h"
#include "app.h"
#include "sprite.h"
#include "player.h"
#include "stage.h"
#include "pang.h"
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
AppData* AppData::s_instance = nullptr;

AppData::AppData()
    : numPlayers(1), numStages(6), currentStage(1), inMenu(true),
      activeScene(nullptr), sharedBackground(nullptr), scrollX(0.0f), 
      scrollY(0.0f), backgroundInitialized(false), debugMode(false),
      quit(false), goBack(false), renderMode(RENDERMODE_NORMAL),
      currentScreen(nullptr), nextScreen(nullptr)
{
    player[PLAYER1] = player[PLAYER2] = nullptr;
    
    // Allocate stages array
    stages = new Stage[6];
}

// Redefine macros after construction
#define quit AppData::instance().quit
#define goback AppData::instance().goBack
#define globalmode AppData::instance().renderMode
#define appGraph graph

AppData& AppData::instance()
{
    if (!s_instance)
    {
        s_instance = new AppData();
    }
    return *s_instance;
}

void AppData::destroy()
{
    if (s_instance)
    {
        s_instance->release();
        delete s_instance;
        s_instance = nullptr;
    }
}

void AppData::init()
{
    inMenu = true;

    // Initialize Player 1 sprites
    bitmaps.player[PLAYER1][0].init(&appGraph, "graph\\p1k1l.png", 0, 3);
    bitmaps.player[PLAYER1][1].init(&appGraph, "graph\\p1k2l.png", 4, 3);
    bitmaps.player[PLAYER1][2].init(&appGraph, "graph\\p1k3l.png", 6, 3);
    bitmaps.player[PLAYER1][3].init(&appGraph, "graph\\p1k4l.png", 4, 3);
    bitmaps.player[PLAYER1][4].init(&appGraph, "graph\\p1k5l.png", 4, 3);
    
    bitmaps.player[PLAYER1][5].init(&appGraph, "graph\\p1k1r.png", 0, 3);
    bitmaps.player[PLAYER1][6].init(&appGraph, "graph\\p1k2r.png", 0, 3);
    bitmaps.player[PLAYER1][7].init(&appGraph, "graph\\p1k3r.png", 0, 3);
    bitmaps.player[PLAYER1][8].init(&appGraph, "graph\\p1k4r.png", 0, 3);
    bitmaps.player[PLAYER1][9].init(&appGraph, "graph\\p1k5r.png", 0, 3);

    bitmaps.player[PLAYER1][10].init(&appGraph, "graph\\p1shoot1.png", 13, 0);
    bitmaps.player[PLAYER1][11].init(&appGraph, "graph\\p1shoot2.png", 13, 3);
    bitmaps.player[PLAYER1][12].init(&appGraph, "graph\\p1win.png", 13, 4);
    bitmaps.player[PLAYER1][13].init(&appGraph, "graph\\p1dead.png");
    bitmaps.player[PLAYER1][14].init(&appGraph, "graph\\p1dead2.png");
    bitmaps.player[PLAYER1][15].init(&appGraph, "graph\\p1dead3.png");
    bitmaps.player[PLAYER1][16].init(&appGraph, "graph\\p1dead4.png");
    bitmaps.player[PLAYER1][17].init(&appGraph, "graph\\p1dead5.png");
    bitmaps.player[PLAYER1][18].init(&appGraph, "graph\\p1dead6.png");
    bitmaps.player[PLAYER1][19].init(&appGraph, "graph\\p1dead7.png");
    bitmaps.player[PLAYER1][20].init(&appGraph, "graph\\p1dead8.png");

    for (int i = 0; i < 21; i++)
        appGraph.setColorKey(bitmaps.player[PLAYER1][i].getBmp(), 0x00FF00);

    // Initialize Player 2 sprites
    bitmaps.player[PLAYER2][0].init(&appGraph, "graph\\p2k1l.png", 0, 3);
    bitmaps.player[PLAYER2][1].init(&appGraph, "graph\\p2k2l.png", 4, 3);
    bitmaps.player[PLAYER2][2].init(&appGraph, "graph\\p2k3l.png", 6, 3);
    bitmaps.player[PLAYER2][3].init(&appGraph, "graph\\p2k4l.png", 4, 3);
    bitmaps.player[PLAYER2][4].init(&appGraph, "graph\\p2k5l.png", 4, 3);
    
    bitmaps.player[PLAYER2][5].init(&appGraph, "graph\\p2k1r.png", 0, 3);
    bitmaps.player[PLAYER2][6].init(&appGraph, "graph\\p2k2r.png", 0, 3);
    bitmaps.player[PLAYER2][7].init(&appGraph, "graph\\p2k3r.png", 0, 3);
    bitmaps.player[PLAYER2][8].init(&appGraph, "graph\\p2k4r.png", 0, 3);
    bitmaps.player[PLAYER2][9].init(&appGraph, "graph\\p2k5r.png", 0, 3);

    bitmaps.player[PLAYER2][10].init(&appGraph, "graph\\p2shoot1.png", 13, 0);
    bitmaps.player[PLAYER2][11].init(&appGraph, "graph\\p2shoot2.png", 13, 3);
    bitmaps.player[PLAYER2][12].init(&appGraph, "graph\\p2win.png", 13, 4);
    bitmaps.player[PLAYER2][13].init(&appGraph, "graph\\p2dead.png");
    bitmaps.player[PLAYER2][14].init(&appGraph, "graph\\p2dead2.png");
    bitmaps.player[PLAYER2][15].init(&appGraph, "graph\\p2dead3.png");
    bitmaps.player[PLAYER2][16].init(&appGraph, "graph\\p2dead4.png");
    bitmaps.player[PLAYER2][17].init(&appGraph, "graph\\p2dead5.png");
    bitmaps.player[PLAYER2][18].init(&appGraph, "graph\\p2dead6.png");
    bitmaps.player[PLAYER2][19].init(&appGraph, "graph\\p2dead7.png");
    bitmaps.player[PLAYER2][20].init(&appGraph, "graph\\p2dead8.png");

    for (int i = 0; i < 21; i++)
        appGraph.setColorKey(bitmaps.player[PLAYER2][i].getBmp(), 0xFF0000);

    // Initialize default key bindings
    playerKeys[PLAYER1].set(SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE);
    playerKeys[PLAYER2].set(SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_LCTRL);
}

void AppData::initStages()
{
    StageExtra extra;
    numStages = 6;

    for (int i = 0; i < numStages; i++)
        stages[i].reset();

    /*************** START OF SCREENS ***********************/
    /***** THE FOLLOWING CODE SHOULD BE DONE BY READING *****/
    /***** FROM A FILE, BUT DUE TO LACK OF TIME         *****/
    /***** WE ARE DOING IT BY HAND.                     *****/

    /* Stage 1 */
    int i = 0;
    stages[i].xpos[PLAYER1] = 250;
    stages[i].xpos[PLAYER2] = 350;
    stages[i].setBack("fondo1.png");
    stages[i].setMusic("stage1.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    stages[i].add(OBJ_FLOOR, 550, 50, 0);		
    stages[i].add(OBJ_FLOOR, 250, 250, 0);
    extra.ex1 = 1;
    stages[i].addX(OBJ_FLOOR, 350, 150, 0, extra);
    stages[i].addX(OBJ_FLOOR, 550, 150, 0, extra);
    extra.ex1 = 0; extra.ex2 = 0; extra.ex3 = 1; extra.ex4 = 1;
    extra.ex1 = 0;
    extra.ex2 = 0;
    extra.ex3 = 1;
    extra.ex4 = 1;
    stages[i].add(OBJ_BALL, 1);

    extra.ex1 = 0;
    extra.ex2 = 0;
    extra.ex3 = 0;
    extra.ex4 = 0;
    stages[i].add(OBJ_BALL, 20);

    /* Stage 2 */
    i = 1;
    stages[i].xpos[PLAYER1] = stages[i].xpos[PLAYER2] = 270;
    stages[i].setBack("fondo2.png");
    stages[i].setMusic("stage4.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    extra.ex1 = 3; extra.ex2 = 200; extra.ex3 = 0; extra.ex4 = 1;
    for (int y = 0; y < 2; y++)
    {
        extra.ex3 = (y == 0) ? -1 : 1;
        for (int x = 0; x < 10; x++)		
            stages[i].addX(OBJ_BALL, 128 + 300 * y + x * 16, 100, 1, extra);
    }

    /* Stage 3 */
    i = 2;
    stages[i].xpos[PLAYER1] = 200;
    stages[i].xpos[PLAYER2] = 350;
    stages[i].setBack("fondo1.png");
    stages[i].setMusic("stage5.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    stages[i].add(OBJ_FLOOR, 250, 70, 0);			
    stages[i].add(OBJ_BALL, 1);  
    stages[i].add(OBJ_BALL, 1);  
    extra.ex1 = 2; extra.ex2 = 0; extra.ex3 = 1; extra.ex4 = 1;
    stages[i].addX(OBJ_BALL, std::rand() % 600 + 32, 400, 1, extra);
    extra.ex3 = -1;
    stages[i].addX(OBJ_BALL, std::rand() % 600 + 32, 400, 1, extra);

    /* Stage 4 */
    i = 3;
    stages[i].setBack("fondo3.png");
    stages[i].setMusic("stage3.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    extra.ex1 = 0;
    stages[i].add(OBJ_FLOOR, 250, 70, 0);	
    extra.ex1 = 3;
    stages[i].addX(OBJ_BALL, 1, extra); 
    stages[i].add(OBJ_BALL, 1); 
    stages[i].add(OBJ_BALL, 20);

    /* Stage 5 */
    i = 4;
    stages[i].xpos[PLAYER1] = 250;
    stages[i].xpos[PLAYER2] = 350;
    stages[i].setBack("fondo5.png");
    stages[i].setMusic("stage4.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    stages[i].add(OBJ_FLOOR, 16, 100, 0);
    stages[i].add(OBJ_FLOOR, 80, 164, 0);
    extra.ex1 = 1;
    stages[i].addX(OBJ_FLOOR, 144, 164, 0, extra);
    stages[i].add(OBJ_FLOOR, 144, 228, 0);
    extra.ex1 = 1;
    stages[i].addX(OBJ_FLOOR, 208, 228, 0, extra);
    stages[i].add(OBJ_FLOOR, 208, 292, 0);
    stages[i].add(OBJ_FLOOR, RES_X - 80, 100, 0);
    stages[i].add(OBJ_FLOOR, RES_X - 128, 164, 0);
    extra.ex1 = 1;
    stages[i].addX(OBJ_FLOOR, RES_X - 144, 164, 0, extra);
    stages[i].add(OBJ_FLOOR, RES_X - 192, 228, 0);
    extra.ex1 = 1;
    stages[i].addX(OBJ_FLOOR, RES_X - 208, 228, 0, extra);
    stages[i].add(OBJ_FLOOR, RES_X - 256, 292, 0);
    extra.ex1 = 3; extra.ex3 = 1; extra.ex4 = 1;		
    for (int x = 0; x < 15; x++)
    {
        extra.ex2 = std::rand() % 150 + 150;
        extra.ex3 = 1;
        stages[i].addX(OBJ_BALL, 17, 50, 5 * x, extra);  		
        extra.ex3 = -1;
        stages[i].addX(OBJ_BALL, MAX_X - 30, 50, 5 * x, extra);  
    }

    /* Stage 6 */
    i = 5;
    stages[i].xpos[PLAYER1] = 250;
    stages[i].xpos[PLAYER2] = 350;
    stages[i].setBack("fondo4.png");
    stages[i].setMusic("stage2.ogg");
    stages[i].timelimit = 100;
    stages[i].id = i + 1;
    extra.ex1 = 1;
    for (int x = 56; x < 600; x += 64)
        for (int y = 22; y < 288; y += 64)
            stages[i].addX(OBJ_FLOOR, x, y, 0, extra);
    extra.ex1 = 1; extra.ex2 = 395; extra.ex3 = 0; extra.ex4 = 1;
    for (int x = 10; x < 650; x += 64)	
        if (x < 250 || x > 350)
        {
            stages[i].addX(OBJ_BALL, x, 395, 1, extra);
            stages[i].addX(OBJ_BALL, x, 150, 1, extra);
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
    
    // Release shared background
    if (backgroundInitialized && sharedBackground)
    {
        sharedBackground->release();
        delete sharedBackground;
        sharedBackground = nullptr;
        backgroundInitialized = false;
    }
    
    // Clean up stages array
    if (stages)
    {
        delete[] stages;
        stages = nullptr;
    }
}

void AppData::preloadMenuMusic()
{
    AudioManager::instance().preloadMusic("music\\menu.ogg");
}

void AppData::preloadStageMusic()
{
    // Preload all stage music tracks
    AudioManager::instance().preloadMusic("music\\stage1.ogg");
    AudioManager::instance().preloadMusic("music\\stage2.ogg");
    AudioManager::instance().preloadMusic("music\\stage3.ogg");
    AudioManager::instance().preloadMusic("music\\stage4.ogg");
    AudioManager::instance().preloadMusic("music\\stage5.ogg");
}
