#pragma once

/**
 * Game Event System - Event Type Definitions and Data Structures
 *
 * Simple enum-based event types with associated data structures.
 * Header-only, no dependencies.
 */

// Forward declarations
class Player;
class Ball;
class Shot;
enum class WeaponType;

/**
 * Game event types
 * Compile-time safe enum for all gameplay events
 */
enum class GameEventType
{
    // High-priority gameplay events
    READY_SCREEN_COMPLETE,    // Ready screen finished blinking, game starts
    LEVEL_CLEAR,              // Stage won
    GAME_OVER,                // Game lost (all players dead or time expired)
    TIME_SECOND_ELAPSED,      // Timer decremented by one second
    STAGE_OBJECT_SPAWNED,     // Ball/floor spawned from stage timeline

    // Entity lifecycle events
    PLAYER_HIT,               // Player killed by ball
    PLAYER_REVIVED,           // Player respawned after death
    BALL_HIT,                 // Ball hit by weapon
    BALL_SPLIT,               // Ball divided into smaller balls

    // Gameplay actions
    PLAYER_SHOOT,             // Player fired weapon
    SCORE_CHANGED,            // Score points added to player
    WEAPON_CHANGED,           // Player switched weapon type

    // Stage events
    STAGE_STARTED,            // New stage initialized
    STAGE_MUSIC_CHANGED,      // Music track changed

    // System events
    CONSOLE_COMMAND           // Console command executed (debug)
};

/**
 * Event-specific data structures
 * Each event type has its own data struct with relevant fields
 */

struct PlayerHitEventData
{
    Player* player;
    Ball* ball;
};

struct BallHitEventData
{
    Ball* ball;
    Shot* shot;
    Player* shooter;
};

struct ScoreChangedEventData
{
    Player* player;
    int scoreAdded;
    int previousScore;
    int newScore;
};

struct PlayerShootEventData
{
    Player* player;
    WeaponType weapon;
};

struct BallSplitEventData
{
    Ball* parentBall;
    int parentSize;
};

struct TimeElapsedEventData
{
    int previousTime;
    int newTime;
};

struct StageStartedEventData
{
    int stageId;
    const char* stageName;
};

struct StageObjectSpawnedEventData
{
    int objectType;  // 0=ball, 1=floor, 2=item
    int x;
    int y;
};

struct LevelClearEventData
{
    int stageId;
};

struct GameOverEventData
{
    int reason;  // 0=player1 dead, 1=both players dead, 2=time expired
};

struct WeaponChangedEventData
{
    Player* player;
    WeaponType previousWeapon;
    WeaponType newWeapon;
};

struct MusicChangedEventData
{
    const char* newMusicFile;
};

struct ConsoleCommandEventData
{
    const char* command;
};

/**
 * Main event data structure
 * Contains event type and union of all event-specific data
 */
struct GameEventData
{
    GameEventType type;
    int timestamp;  // SDL_GetTicks() when event fired

    // Union for memory efficiency - only one event data active at a time
    union
    {
        PlayerHitEventData playerHit;
        BallHitEventData ballHit;
        ScoreChangedEventData scoreChanged;
        PlayerShootEventData playerShoot;
        BallSplitEventData ballSplit;
        TimeElapsedEventData timeElapsed;
        StageStartedEventData stageStarted;
        StageObjectSpawnedEventData objectSpawned;
        LevelClearEventData levelClear;
        GameOverEventData gameOver;
        WeaponChangedEventData weaponChanged;
        MusicChangedEventData musicChanged;
        ConsoleCommandEventData consoleCommand;
    };

    // Default constructor
    GameEventData() : type(GameEventType::READY_SCREEN_COMPLETE), timestamp(0) {}
};
