#pragma once

#include <SDL.h>
#include "sprite.h"
#include "graph.h"
#include "minput.h"
#include "configdata.h"

// Forward declarations
class Player;
class Sprite;
class Stage;
class GameState;

/**
 * Keys class
 * Stores keyboard mapping for each player.
 */
class Keys
{
public:
    SDL_Scancode left;
    SDL_Scancode right;
    SDL_Scancode shoot;

    void setLeft(SDL_Scancode l) { left = l; }
    void setRight(SDL_Scancode r) { right = r; }
    void setShoot(SDL_Scancode s) { shoot = s; }
    void set(SDL_Scancode lf, SDL_Scancode rg, SDL_Scancode sh)
    {
        left = lf;
        right = rg;
        shoot = sh;
    }
};

/**
 * GameBitmaps struct
 * Stores player sprites.
 */
struct GameBitmaps
{
    Sprite player[2][21];
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
    static AppData* s_instance;

    // Private constructor for singleton
    AppData();
    
    // Delete copy constructor and assignment operator
    AppData(const AppData&) = delete;
    AppData& operator=(const AppData&) = delete;

public:
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
    Player* player[2];
    Keys playerKeys[2];
    GameBitmaps bitmaps;
    int currentStage;
    Stage* stages; // Dynamic array
    bool inMenu;
    GameState* activeScene;

    // Shared background resources (moved from GameState statics)
    Sprite* sharedBackground;
    float scrollX;
    float scrollY;
    bool backgroundInitialized;

    // Global flags (moved from GameState statics)
    bool debugMode;

    // Initialization methods
    void init();
    void initStages();
    void setCurrent(GameState* state);
    void release();

    // Accessors for convenience (to ease migration)
    Player** getPlayers() { return player; }
    Keys* getKeys() { return playerKeys; }
    GameBitmaps& getBmp() { return bitmaps; }
    Stage* getStages() { return stages; }
    int& getCurrentStage() { return currentStage; }
    int& getNumPlayers() { return numPlayers; }
    int& getNumStages() { return numStages; }
    bool& isMenu() { return inMenu; }
};
