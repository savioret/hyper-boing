#include "appdata.h"
#include "app.h"
#include "sprite.h"
#include "player.h"
#include "stage.h"
#include "main.h"
#include "logger.h"
#include "asepriteloader.h"
#include "animspritesheet.h"
#include <cstdio>
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


void Keys::setLeft(SDL_Scancode l) 
{ 
    left = l;
    LOG_DEBUG("Set Left key to %d %s", l, Keys::getKeyName(l));
}

void Keys::setRight(SDL_Scancode r) 
{ 
    right = r;
    LOG_DEBUG("Set Right key to %d %s", r, Keys::getKeyName(r));
}

void Keys::setShoot(SDL_Scancode s)
{
    shoot = s;
    LOG_DEBUG("Set Shoot key to %d %s", s, Keys::getKeyName(s));
}

void Keys::setUp(SDL_Scancode u)
{
    up = u;
    LOG_DEBUG("Set Up key to %d %s", u, Keys::getKeyName(u));
}

void Keys::setDown(SDL_Scancode d)
{
    down = d;
    LOG_DEBUG("Set Down key to %d %s", d, Keys::getKeyName(d));
}

void Keys::set(SDL_Scancode lf, SDL_Scancode rg, SDL_Scancode sh, SDL_Scancode up, SDL_Scancode dn)
{
    setLeft(lf);
    setRight(rg);
    setShoot(sh);
    setUp(up);
    setDown(dn);
}

const char* Keys::getKeyName(SDL_Scancode scancode)
{
    const char* name = SDL_GetScancodeName(scancode);
    return name ? name : "Unknown";
}



AppData::AppData()
    : numPlayers(1), numStages(0), currentStage(1), inMenu(true),
      activeScene(nullptr), sharedBackground(nullptr), scrollX(0.0f),
      scrollY(0.0f), backgroundInitialized(false), debugMode(false),
      quit(false), goBack(false), renderMode(RENDERMODE_NORMAL),
      currentScreen(nullptr), nextScreen(nullptr)
{
    player[PLAYER1] = nullptr;
    player[PLAYER2] = nullptr;
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

    // Initialize default key bindings (left, right, shoot, up, down)
    playerKeys[PLAYER1].set(SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE,
                            SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);
    playerKeys[PLAYER2].set(SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_LCTRL,
                            SDL_SCANCODE_W, SDL_SCANCODE_S);
}

/**
 * @brief Loads shared stage resources once at startup
 * 
 * These sprites are used by all stages and are kept in memory to avoid
 * redundant loading/unloading between stage transitions. Called once
 * during application initialization.
 */
void AppData::initStageResources()
{
    if (stageRes.initialized)
        return;  // Already loaded

    int i;

    // Load ball sprites per color (0=red, 1=green, 2=blue)
    static const char* BALL_JSON        = "assets/graph/entities/ball.json";
    static const char* BALL_SPLASH_JSON = "assets/graph/entities/ball_splash.json";
    stageRes.ballAnim[0] = AnimSpriteSheet::load(&appGraph, BALL_JSON, "assets/graph/entities/ball_red.png");
    stageRes.ballAnim[1] = AnimSpriteSheet::load(&appGraph, BALL_JSON, "assets/graph/entities/ball_green.png");
    stageRes.ballAnim[2] = AnimSpriteSheet::load(&appGraph, BALL_JSON, "assets/graph/entities/ball_blue.png");
    stageRes.ballSplashAnim[0] = AnimSpriteSheet::load(&appGraph, BALL_SPLASH_JSON, "assets/graph/entities/ball_splash_red.png");
    stageRes.ballSplashAnim[1] = AnimSpriteSheet::load(&appGraph, BALL_SPLASH_JSON, "assets/graph/entities/ball_splash_green.png");
    stageRes.ballSplashAnim[2] = AnimSpriteSheet::load(&appGraph, BALL_SPLASH_JSON, "assets/graph/entities/ball_splash_blue.png");

    // Load mini player icons
    stageRes.miniplayer[PLAYER1].init(&appGraph, "assets/graph/players/miniplayer1.png");
    stageRes.miniplayer[PLAYER2].init(&appGraph, "assets/graph/players/miniplayer2.png");

    // Load lives icons
    stageRes.lives[PLAYER1].init(&appGraph, "assets/graph/players/lives1p.png");
    stageRes.lives[PLAYER2].init(&appGraph, "assets/graph/players/lives2p.png");
    appGraph.setColorKey(stageRes.lives[PLAYER1].getBmp(), 0x00FF00);
    appGraph.setColorKey(stageRes.lives[PLAYER2].getBmp(), 0x00FF00);

    // Load harpoon sprites
    stageRes.harpoonTip.init(&appGraph, "assets/graph/entities/harpoon_tip.png");
    appGraph.setColorKey(stageRes.harpoonTip.getBmp(), 0x00FF00);

    // Load harpoon chain animated sprite sheet
    stageRes.harpoonAnim = AsepriteLoader::load(&appGraph, "assets/graph/entities/harpoon_chain.json", stageRes.harpoonChain);

    // Load gun bullet sprite sheet with animation data
    stageRes.gunBulletAnim = AnimSpriteSheet::loadAsStateMachine(&appGraph, "assets/graph/entities/gun_bullet.json");

    // Load claw weapon sprite sheets
    stageRes.clawWeaponAnim = AnimSpriteSheet::load(&appGraph, "assets/graph/entities/claw_weapon.json");
    stageRes.clawWeaponYellowAnim = AnimSpriteSheet::load(&appGraph, "assets/graph/entities/claw_weapon_yellow.json");

    // Load gun muzzle flash effect
    stageRes.gunSparkAnim = AnimSpriteSheet::load(&appGraph, "assets/graph/entities/gun_spark.json");

    // Load harpoon muzzle flash effect
    stageRes.harpoonSparkAnim = AnimSpriteSheet::load(&appGraph, "assets/graph/entities/harpoon_spark.json");

    // Load border markers
    stageRes.mark[0].init(&appGraph, "assets/graph/entities/ladrill1.png");
    stageRes.mark[1].init(&appGraph, "assets/graph/entities/ladrill1u.png");
    stageRes.mark[2].init(&appGraph, "assets/graph/entities/ladrill1d.png");
    stageRes.mark[3].init(&appGraph, "assets/graph/entities/ladrill1l.png");
    stageRes.mark[4].init(&appGraph, "assets/graph/entities/ladrill1r.png");
    for (i = 0; i < 5; i++)
        appGraph.setColorKey(stageRes.mark[i].getBmp(), 0x00FF00);

    // Load floor sprite sheets per color (0=red, 1=blue, 2=green, 3=yellow)
    static const char* FLOOR_JSON = "assets/graph/entities/floor.json";
    stageRes.floorAnim[0] = AnimSpriteSheet::load(&appGraph, FLOOR_JSON, "assets/graph/entities/floor_red.png");
    stageRes.floorAnim[1] = AnimSpriteSheet::load(&appGraph, FLOOR_JSON, "assets/graph/entities/floor_blue.png");
    stageRes.floorAnim[2] = AnimSpriteSheet::load(&appGraph, FLOOR_JSON, "assets/graph/entities/floor_green.png");
    stageRes.floorAnim[3] = AnimSpriteSheet::load(&appGraph, FLOOR_JSON, "assets/graph/entities/floor_yellow.png");

    // Load ladder sprite
    stageRes.ladder.init(&appGraph, "assets/graph/entities/ladder.png");

    // Load UI sprites
    stageRes.time.init(&appGraph, "assets/graph/ui/tiempo.png");

    stageRes.gameover.init(&appGraph, "assets/graph/ui/gameover.png", 16, 16);
    stageRes.continu.init(&appGraph, "assets/graph/ui/continue.png", 16, 16);
    stageRes.ready.init(&appGraph, "assets/graph/ui/ready.png", 16, 16);

    // Players sprites
    bitmaps.player[PLAYER1][5].init(&appGraph, "assets/graph/players/p1shoot1.png", 0, 0);
    bitmaps.player[PLAYER1][6].init(&appGraph, "assets/graph/players/p1shoot2.png", 0, 3);
    bitmaps.player[PLAYER1][7].init(&appGraph, "assets/graph/players/p1win.png", 0, 4);
    bitmaps.player[PLAYER1][8].init(&appGraph, "assets/graph/players/p1dead.png");

    bitmaps.player[PLAYER2][5].init(&appGraph, "assets/graph/players/p2shoot1.png", 0, 0);
    bitmaps.player[PLAYER2][6].init(&appGraph, "assets/graph/players/p2shoot2.png", 0, 3);
    bitmaps.player[PLAYER2][7].init(&appGraph, "assets/graph/players/p2win.png", 0, 4);
    bitmaps.player[PLAYER2][8].init(&appGraph, "assets/graph/players/p2dead.png");


    // Load font sprites
    int offs[10] = { 0, 22, 44, 71, 93, 120, 148, 171, 198, 221 };
    int offs1[10] = { 0, 13, 18, 31, 44, 58, 70, 82, 93, 105 };
    int offs2[10] = { 0, 49, 86, 134, 187, 233, 277, 327, 374, 421 };

    stageRes.fontnum[0].init(&appGraph, "assets/graph/ui/fontnum1.png", 0, 0);
    appGraph.setColorKey(stageRes.fontnum[0].getBmp(), 0xFF0000);
    
    stageRes.fontnum[1].init(&appGraph, "assets/graph/ui/fontnum2.png", 0, 0);
    appGraph.setColorKey(stageRes.fontnum[1].getBmp(), 0xFF0000);
    
    stageRes.fontnum[2].init(&appGraph, "assets/graph/ui/fontnum3.png", 0, 0);
    appGraph.setColorKey(stageRes.fontnum[2].getBmp(), 0x00FF00);

    // Preload ball pop sound effects (optional - gracefully handles missing files)
    // pop1 = largest balls (size 0), pop2 = medium balls (size 1-2), pop3 = smallest balls (size 3)
    // Note: registerSound will log errors but won't crash if files don't exist
    AudioManager::instance().registerSound("pop1", "assets/sounds/pop1.ogg");
    AudioManager::instance().registerSound("pop2", "assets/sounds/pop2.ogg");
    AudioManager::instance().registerSound("pop3", "assets/sounds/pop3.ogg");

    // Preload weapon sound effects
    AudioManager::instance().registerSound("harpoon", "assets/sounds/harpoon.ogg");
    AudioManager::instance().registerSound("gun", "assets/sounds/gun.ogg");
    AudioManager::instance().registerSound("claw", "assets/sounds/claw.ogg");

    // Preload player sound effects
    AudioManager::instance().registerSound("player_dies", "assets/sounds/player_dies.ogg");

    // Preload pickup sound effect
    AudioManager::instance().registerSound("pickup", "assets/sounds/pickup.ogg");

    // Load pickup sprites
    stageRes.pickupSprites[0].init(&appGraph, "assets/graph/entities/pickup_gun.png");
    stageRes.pickupSprites[1].init(&appGraph, "assets/graph/entities/pickup_doubleshoot.png");
    stageRes.pickupSprites[2].init(&appGraph, "assets/graph/entities/pickup_extratime.png");
    stageRes.pickupSprites[3].init(&appGraph, "assets/graph/entities/pickup_timefreeze.png");
    stageRes.pickupSprites[4].init(&appGraph, "assets/graph/entities/pickup_1up.png");
    stageRes.pickupShieldAnim = AnimSpriteSheet::load(&appGraph, "assets/graph/entities/pickup_shield.json");
    stageRes.pickupSprites[5].init(&appGraph, "assets/graph/entities/pickup_claw.png");
    stageRes.shieldAnim = AnimSpriteSheet::load(&appGraph, "assets/graph/entities/shield_anim.json");
    stageRes.itemHolder.init(&appGraph, "assets/graph/ui/item_holder.png");

    // Load glass sprite sheets per color (0=red, 1=blue, 2=green, 3=yellow)
    static const char* GLASS_JSON = "assets/graph/entities/glass.json";
    stageRes.glassAnim[0] = AnimSpriteSheet::load(&appGraph, GLASS_JSON, "assets/graph/entities/glass_red.png");
    stageRes.glassAnim[1] = AnimSpriteSheet::load(&appGraph, GLASS_JSON, "assets/graph/entities/glass_blue.png");
    stageRes.glassAnim[2] = AnimSpriteSheet::load(&appGraph, GLASS_JSON, "assets/graph/entities/glass_green.png");
    stageRes.glassAnim[3] = AnimSpriteSheet::load(&appGraph, GLASS_JSON, "assets/graph/entities/glass_yellow.png");

    // Load hexa enemy sprites per color (0=green, 1=cyan, 2=orange, 3=purple)
    static const char* HEXA_JSON         = "assets/graph/entities/hexa.json";
    static const char* HEXA_SPLASH_JSON  = "assets/graph/entities/hexagon_splash.json";
    stageRes.hexaAnim[0] = AnimSpriteSheet::load(&appGraph, HEXA_JSON, "assets/graph/entities/hexa_green.png");
    stageRes.hexaAnim[1] = AnimSpriteSheet::load(&appGraph, HEXA_JSON, "assets/graph/entities/hexa_cyan.png");
    stageRes.hexaAnim[2] = AnimSpriteSheet::load(&appGraph, HEXA_JSON, "assets/graph/entities/hexa_orange.png");
    stageRes.hexaAnim[3] = AnimSpriteSheet::load(&appGraph, HEXA_JSON, "assets/graph/entities/hexa_purple.png");
    stageRes.hexaSplashAnim[0] = AnimSpriteSheet::load(&appGraph, HEXA_SPLASH_JSON, "assets/graph/entities/hexagon_splash_green.png");
    stageRes.hexaSplashAnim[1] = AnimSpriteSheet::load(&appGraph, HEXA_SPLASH_JSON, "assets/graph/entities/hexagon_splash_cyan.png");
    stageRes.hexaSplashAnim[2] = AnimSpriteSheet::load(&appGraph, HEXA_SPLASH_JSON, "assets/graph/entities/hexagon_splash_orange.png");
    stageRes.hexaSplashAnim[3] = AnimSpriteSheet::load(&appGraph, HEXA_SPLASH_JSON, "assets/graph/entities/hexagon_splash_purple.png");

    stageRes.initialized = true;
}

void AppData::initStages()
{
    stages.clear();

    char path[260];
    for (int i = 1; ; i++)
    {
        std::snprintf(path, sizeof(path), "assets/stages/stage%d.stg", i);
        FILE* f = std::fopen(path, "r");
        if (!f)
            break;
        std::fclose(f);
        stages.emplace_back();
        stages.back().stageFile = path;
    }

    numStages = (int)stages.size();
    LOG_INFO("Found %d stage(s) in assets/stages/", numStages);
}
void AppData::setCurrent(GameState* state)
{
    activeScene = state;
}

void AppData::release()
{
    // Release player bitmaps
    for (int i = 0; i < 9; i++)
    {
        bitmaps.player[PLAYER1][i].release();
        bitmaps.player[PLAYER2][i].release();
    }
    
    // Release shared stage resources
    if (stageRes.initialized)
    {
        for (int i = 0; i < 2; i++)
        {
            stageRes.miniplayer[i].release();
            stageRes.lives[i].release();
        }
        stageRes.ladder.release();

        // Release weapon sprites
        stageRes.harpoonTip.release();
        stageRes.harpoonChain.release();

        for (int i = 0; i < 3; i++)
        {
            stageRes.fontnum[i].release();
        }
        for (int i = 0; i < 5; i++)
            stageRes.mark[i].release();
        
        stageRes.time.release();
        stageRes.gameover.release();
        stageRes.continu.release();
        stageRes.ready.release();

        // Release pickup sprites
        for (int i = 0; i < 6; i++)
            stageRes.pickupSprites[i].release();

        stageRes.initialized = false;
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
    AudioManager::instance().preloadMusic("assets/music/menu.ogg");
}

