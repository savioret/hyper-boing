#pragma once

#include <memory>
#include <vector>
#include <SDL.h>
#include "sprite.h"
#include "spritesheet.h"
#include "animcontroller.h"
#include "animspritesheet.h"
#include "graph.h"
#include "minput.h"
#include "configdata.h"
#include "oncehelper.h"

// Forward declarations
class Player;
class Sprite;
class Stage;
class GameState;

/**
 * Keys class
 * Stores keyboard mapping for each player.
 * Members are now private with proper getters/setters for encapsulation.
 */
class Keys
{
private:
    SDL_Scancode left;
    SDL_Scancode right;
    SDL_Scancode shoot;
    SDL_Scancode up;
    SDL_Scancode down;

public:
    // Getters
    SDL_Scancode getLeft() const { return left; }
    SDL_Scancode getRight() const { return right; }
    SDL_Scancode getShoot() const { return shoot; }
    SDL_Scancode getUp() const { return up; }
    SDL_Scancode getDown() const { return down; }

    // Setters
    void setLeft(SDL_Scancode l);
    void setRight(SDL_Scancode r);
    void setShoot(SDL_Scancode s);
    void setUp(SDL_Scancode u);
    void setDown(SDL_Scancode d);
    void set(SDL_Scancode lf, SDL_Scancode rg, SDL_Scancode sh, SDL_Scancode up, SDL_Scancode dn);
    static const char* getKeyName(SDL_Scancode scancode);
};

/**
 * @struct GameBitmaps
 * @brief Stores player sprites
 */
struct GameBitmaps
{
    Sprite player[2][9];  ///< Player sprites (2 players, 9 frames each)
};

/**
 * @struct StageResources
 * @brief Shared sprites used across all stages
 * 
 * These resources are loaded once at startup and reused by all scenes,
 * avoiding redundant loading/unloading between stage transitions.
 */
struct StageResources
{
    // Ball sprites
    Sprite redball[4];  ///< Ball sprites (4 sizes)
    
    // Floor sprites
    Sprite floor[2];    ///< Floor sprites (0=horizontal, 1=vertical)

    // Ladder sprite
    Sprite ladder;      ///< Ladder tile sprite (tiled vertically)

    // Weapon sprites
    Sprite harpoonTip;                        ///< Harpoon tip sprite
    SpriteSheet harpoonChain;                 ///< Harpoon chain animated sprite sheet (shared texture)
    std::unique_ptr<IAnimController> harpoonAnim;  ///< Template animation for harpoon chain (cloned per shot)
    std::unique_ptr<AnimSpriteSheet> gunBulletAnim;  ///< Gun bullet sprite sheet with animation (cloned per shot)

    // Effect animation templates (cloned per instance)
    std::unique_ptr<AnimSpriteSheet> ballPopAnim[3];   ///< Ball pop effects: [0]=size0, [1]=size1, [2]=size2+
    std::unique_ptr<AnimSpriteSheet> gunSparkAnim;     ///< Gun muzzle flash effect
    std::unique_ptr<AnimSpriteSheet> harpoonSparkAnim; ///< Harpoon muzzle flash effect
    
    // Border/marker sprites
    Sprite mark[5];     ///< Border marker sprites
    
    // UI sprites
    Sprite miniplayer[2];  ///< Mini player icons for HUD
    Sprite lives[2];       ///< Lives icons for HUD
    Sprite time;           ///< Time icon
    Sprite gameover;       ///< Game over text
    Sprite continu;        ///< Continue text
    Sprite ready;          ///< Ready text
    
    // Font sprites
    Sprite fontnum[3];     ///< Number fonts (3 sizes)
    
    bool initialized;      ///< True if resources have been loaded
    
    StageResources() : initialized(false) {}
};

/**
 * AppData class (Singleton)
 *
 * Central repository for global application data, configuration,
 * and shared resources. Replaces scattered global variables with
 * a single, controlled access point.
 *
 * Responsibilities:
 * - Game session data (players, scores, stages)
 * - Shared resources (bitmaps, background)
 * - System instances (graphics, input, config)
 * - Global flags (debug mode, menu state)
 */
class AppData
{
private:
    // Singleton instance
    static std::unique_ptr<AppData> s_instance;

    // Private constructor for singleton
    AppData();
    
    // Delete copy constructor and assignment operator
    AppData(const AppData&) = delete;
    AppData& operator=(const AppData&) = delete;
    
    // Friend declaration for make_unique
    friend std::unique_ptr<AppData> std::make_unique<AppData>();

    // Game-level once helper (persists across stages)
    OnceHelper gameOnceHelper;

public:
    // Player indices
    static constexpr int PLAYER1 = 0;
    static constexpr int PLAYER2 = 1;

    // Singleton accessor
    static AppData& instance();
    static void destroy();

    // System instances (moved from globals)
    Graph graph;
    MInput input;
    ConfigData config;

    // Game session data (from GameInfo)
    int numPlayers;
    int numStages;
    std::unique_ptr<Player> player[2];
    Keys playerKeys[2];
    GameBitmaps bitmaps;
    StageResources stageRes;  ///< Shared stage resources (loaded once)
    int currentStage;
    std::vector<Stage> stages;
    bool inMenu;
    GameState* activeScene;

    // Shared background resources (moved from GameState statics)
    std::unique_ptr<Sprite> sharedBackground;
    float scrollX;
    float scrollY;
    bool backgroundInitialized;

    // Global flags and state (moved from main)
    bool debugMode;
    bool quit;           // Application quit flag
    bool goBack;         // Return to menu flag
    int renderMode;      // Render mode (windowed/fullscreen)
    std::unique_ptr<GameState> currentScreen;  // Current active screen
    std::unique_ptr<GameState> nextScreen;     // Next screen to transition to

    // Initialization methods
    void init();
    void initStages();
    void initStageResources();  ///< Load shared stage sprites once
    void setCurrent(GameState* state);
    void release();
    
    // Music preloading methods
    void preloadMenuMusic();
    void preloadStageMusic();

    // Accessors for convenience (to ease migration)
    Player* getPlayer(int p) { return reinterpret_cast<Player**>(player)[p]; }
    Keys& getKeys(int playerIndex) { return playerKeys[playerIndex]; }
    GameBitmaps& getBmp() { return bitmaps; }
    StageResources& getStageRes() { return stageRes; }  ///< Get shared stage resources
    Stage* getStages() { return stages.data(); }
    int& getCurrentStage() { return currentStage; }
    int& getNumPlayers() { return numPlayers; }
    int& getNumStages() { return numStages; }
    bool& isMenu() { return inMenu; }

    // Game-level "once" helper (persists across all stages)
    bool gameOnce(const std::string& key) {
        return gameOnceHelper.once(key);
    }

    OnceHelper& getGameOnceHelper() { return gameOnceHelper; }
};
