#pragma once

#include <memory>
#include <vector>
#include <SDL.h>
#include "sprite.h"
#include "graph.h"
#include "minput.h"
#include "configdata.h"
#include "oncehelper.h"
#include "stageresources.h"

// Forward declarations
class Player;
class Stage;
class GameState;
class AnimSpriteSheet;
class SpriteSheet;
class IAnimController;
class AnimController;

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
