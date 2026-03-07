#include "appconsole.h"
#include "graph.h"
#include "sprite.h"
#include "bmfont.h"
#include "logger.h"
#include "main.h"
#include "eventmanager.h"
#include <algorithm>
#include <sstream>

// Initialize static singleton instance
std::unique_ptr<AppConsole> AppConsole::s_instance = nullptr;

AppConsole::AppConsole()
    : graph(nullptr), fontRenderer(nullptr),
      fontOwned(false), visible(false), initialized(false), overlayAlpha(0.85f),
      cursorPosition(0), cursorVisible(true), cursorBlinkTime(0), historyIndex(-1),
      maxVisibleLines(20), scrollOffset(0), fontSize(10), lineHeight(12), 
      padding(10), promptHeight(24)
{
}

AppConsole& AppConsole::instance()
{
    if (!s_instance)
    {
        s_instance = std::make_unique<AppConsole>();
    }
    return *s_instance;
}

void AppConsole::destroy()
{
    if (s_instance)
    {
        s_instance->release();
        s_instance.reset();
    }
}

bool AppConsole::init(Graph* gr)
{
    if (initialized)
        return true;
    
    graph = gr;
    
    // SIMPLIFIED: Load font with one call
    fontRenderer = std::make_unique<BMFontRenderer>();
    if (!fontRenderer->loadFont(graph, "assets/fonts/monospaced_10.fnt"))
    {
        LOG_WARNING("AppConsole: Could not load monospaced_10.fnt, trying fallback");
        
        // Try fallback font
        if (!fontRenderer->loadFont(graph, "assets/fonts/thickfont_grad_64.fnt"))
        {
            LOG_ERROR("AppConsole: Failed to load any font, using system font");
            // fontRenderer will use system font as fallback
        }
    }
    
    fontRenderer->setScale(1.0f);
    
    fontOwned = true;  // We own the fontRenderer which manages everything internally
    initialized = true;
    
    // Register built-in commands
    registerBuiltinCommands();
    
    LOG_SUCCESS("AppConsole initialized");
    
    return true;
}

bool AppConsole::initWithFont(Graph* gr, BMFontLoader* font, Sprite* texture)
{
    if (initialized)
        return true;
    
    graph = gr;
    
    // Create font renderer with external resources (legacy API)
    fontRenderer = std::make_unique<BMFontRenderer>();
    fontRenderer->init(graph, font, texture);
    fontRenderer->setScale(1.0f);
    
    fontOwned = false;  // We don't own the font resources
    initialized = true;
    
    // Register built-in commands
    registerBuiltinCommands();
    
    LOG_SUCCESS("AppConsole initialized with external font");
    
    return true;
}

void AppConsole::release()
{
    if (fontOwned && fontRenderer)
    {
        fontRenderer->release();
    }

    fontRenderer.reset();
    graph = nullptr;

    commands.clear();
    commandHistory.clear();
    inputBuffer.clear();

    initialized = false;
}

void AppConsole::show()
{
    if (!visible)
    {
        visible = true;
        inputBuffer.clear();
        cursorPosition = 0;
        // Ensure text input is enabled (should already be active, but ensure it)
        SDL_StartTextInput();
    }
}

void AppConsole::hide()
{
    if (visible)
    {
        visible = false;
        inputBuffer.clear();
        cursorPosition = 0;
        // Don't call SDL_StopTextInput() - keep text input active so "/" shortcut continues to work
        // Text input events are filtered by handleEvent() when console is hidden
    }
}

void AppConsole::toggle()
{
    if (visible)
        hide();
    else
        show();
}

void AppConsole::registerBuiltinCommands()
{
    registerCommand("help", "Show available commands", 
        [this](const std::string& args) { cmdHelp(args); });
    
    registerCommand("echo", "Echo text to console", 
        [this](const std::string& args) { cmdEcho(args); });
    
    registerCommand("clear", "Clear console log", 
        [this](const std::string& args) { cmdClear(args); });
    
    registerCommand("level", "Set log level (trace/debug/info/warning/error)", 
        [this](const std::string& args) { cmdLevel(args); });
    
    registerCommand("next", "Skip to next level (or /next <stage>) with stage-clear animation", 
        [this](const std::string& args) { cmdNext(args); });

    registerCommand("goto", "Ultra-fast stage switch (no stage-clear): /goto [stage]",
        [this](const std::string& args) { cmdGoto(args); });

    registerCommand("weapon", "Switch weapon: /weapon <harpoon|harpoon2|gun|claw> [player_num]",
        [this](const std::string& args) { cmdWeapon(args); });

    registerCommand("boxes", "Toggle bounding boxes: /boxes <1|0>",
        [this](const std::string& args) { cmdBoxes(args); });

    registerCommand("events", "Toggle event logging: /events",
        [this](const std::string& args) { cmdEvents(args); });

    registerCommand("time", "Set stage countdown time: /time <seconds>",
        [this](const std::string& args) { cmdTime(args); });
    
    registerCommand("lives", "Set player lives: /lives <player_num> <lives>",
        [this](const std::string& args) { cmdLives(args); });

    registerCommand("ball", "Spawn a ball: /ball <x> <y> [size] [count] (-1 for random position)",
        [this](const std::string& args) { cmdBall(args); });

    registerCommand("floor", "Spawn a floor: /floor <x> <y> [type] (type: string or int 0-4)",
        [this](const std::string& args) { cmdFloor(args); });

    registerCommand("immune", "Toggle player immunity: /immune [player_num] [0|1]",
        [this](const std::string& args) { cmdImmune(args); });

    registerCommand("shield", "Toggle player shield: /shield [player_num] [0|1]",
        [this](const std::string& args) { cmdShield(args); });
}

void AppConsole::cmdHelp(const std::string& args)
{
    LOG_INFO("=== Available Commands ===");
    for (const ConsoleCommand& cmd : commands)
    {
        LOG_INFO("  /%s - %s", cmd.name.c_str(), cmd.description.c_str());
    }
}

void AppConsole::cmdEcho(const std::string& args)
{
    if (args.empty())
    {
        LOG_WARNING("Usage: /echo <text>");
        return;
    }
    LOG_INFO("%s", args.c_str());
}

void AppConsole::cmdClear(const std::string& args)
{
    Logger::instance().clearEntries();
    LOG_INFO("Console cleared");
}

void AppConsole::cmdLevel(const std::string& args)
{
    if (args.empty())
    {
        LOG_WARNING("Usage: /level <trace|debug|info|warning|error>");
        return;
    }
    
    std::string levelStr = args;
    std::transform(levelStr.begin(), levelStr.end(), levelStr.begin(), ::tolower);
    
    if (levelStr == "trace")
        Logger::instance().setMinLevel(LogLevel::TRACE);
    else if (levelStr == "debug")
        Logger::instance().setMinLevel(LogLevel::DEBUG);
    else if (levelStr == "info")
        Logger::instance().setMinLevel(LogLevel::INFO);
    else if (levelStr == "warning" || levelStr == "warn")
        Logger::instance().setMinLevel(LogLevel::WARNING);
    else if (levelStr == "error" || levelStr == "err")
        Logger::instance().setMinLevel(LogLevel::ERR);
    else
    {
        LOG_WARNING("Unknown log level: %s", args.c_str());
        return;
    }
    
    LOG_SUCCESS("Log level set to: %s", levelStr.c_str());
}

void AppConsole::cmdNext(const std::string& args)
{
    AppData& appData = AppData::instance();
    
    // Check if we're in a Scene (gameplay screen)
    Scene* currentScene = dynamic_cast<Scene*>(appData.currentScreen.get());
    
    if (!currentScene)
    {
        LOG_WARNING("/next can only be used during gameplay (Scene)");
        return;
    }

    int targetStage = appData.currentStage + 1;

    if (!args.empty())
    {
        try
        {
            targetStage = std::stoi(args);
        }
        catch (const std::exception&)
        {
            LOG_WARNING("Invalid stage number: %s (must be 1-%d)", args.c_str(), appData.numStages);
            return;
        }
    }

    if (targetStage < 1 || targetStage > appData.numStages)
    {
        LOG_WARNING("Stage %d is out of range (valid: 1-%d)", targetStage, appData.numStages);
        return;
    }

    if (targetStage == appData.currentStage)
    {
        LOG_INFO("Already on level %d", targetStage);
        return;
    }

    LOG_SUCCESS("Skipping to level %d (with stage clear)...", targetStage);
    currentScene->skipToStage(targetStage);
}

void AppConsole::cmdGoto(const std::string& args)
{
    AppData& appData = AppData::instance();

    Scene* currentScene = dynamic_cast<Scene*>(appData.currentScreen.get());
    if (!currentScene)
    {
        LOG_WARNING("/goto can only be used during gameplay (Scene)");
        return;
    }

    if (args.empty())
    {
        // No argument: ultra-fast jump to next stage
        queueQuickStageSwitchByOffset(1);
        return;
    }

    // Optional argument: ultra-fast jump to a specific stage number
    int targetStage = 0;
    try
    {
        targetStage = std::stoi(args);
    }
    catch (const std::exception&)
    {
        LOG_WARNING("Invalid stage number: %s (must be 1-%d)", args.c_str(), appData.numStages);
        return;
    }

    if (targetStage < 1 || targetStage > appData.numStages)
    {
        LOG_WARNING("Stage %d is out of range (valid: 1-%d)", targetStage, appData.numStages);
        return;
    }

    if (targetStage == appData.currentStage)
    {
        LOG_INFO("Already on level %d", targetStage);
        return;
    }

    currentScene->queueQuickStageSwitch(targetStage);
    LOG_SUCCESS("Ultra-fast switch queued to level %d", targetStage);
}

/**
 * Command: /weapon <harpoon|gun|claw>
 *
 * Switches the player's weapon.
 * Optionally specify player number: /weapon <weapon> <player_num>
 *   player_num: 1 or 2 (omit to change both players)
 */
void AppConsole::cmdWeapon(const std::string& args)
{
    if (args.empty())
    {
        LOG_WARNING("Usage: /weapon <harpoon|gun|claw> [player_number]");
        LOG_INFO("  player_number: 1 or 2 (omit to change both players)");
        return;
    }

    // Get current scene
    AppData& appData = AppData::instance();
    Scene* scene = dynamic_cast<Scene*>(appData.currentScreen.get());
    if (!scene)
    {
        LOG_WARNING("Weapon switching only available during gameplay");
        return;
    }

    // Parse arguments
    std::istringstream iss(args);
    std::string weaponName;
    int playerNum = -1;  // -1 means both players
    iss >> weaponName >> playerNum;

    // Convert to lowercase for comparison
    std::transform(weaponName.begin(), weaponName.end(), weaponName.begin(), ::tolower);

    // Map string to WeaponType
    WeaponType type;
    if (weaponName == "harpoon")
    {
        type = WeaponType::HARPOON;
    }
    else if (weaponName == "gun")
    {
        type = WeaponType::GUN;
    }
    else if (weaponName == "claw")
    {
        type = WeaponType::CLAW;
    }
    else
    {
        LOG_ERROR("Unknown weapon: %s", weaponName.c_str());
        LOG_INFO("Available weapons: harpoon, harpoon2, gun, claw");
        return;
    }

    // Apply to specified player(s)
    bool changed = false;
    if (playerNum == -1 || playerNum == 1)
    {
        Player* p1 = scene->getPlayer(0);
        if (p1)
        {
            p1->setWeapon(type);
            LOG_SUCCESS("Player 1 weapon set to: %s", weaponName.c_str());
            changed = true;
        }
    }
    if (playerNum == -1 || playerNum == 2)
    {
        // TODO: This seems to crash when no player 2 exists
        Player* p2 = scene->getPlayer(1);
        if (p2)
        {
            p2->setWeapon(type);
            LOG_SUCCESS("Player 2 weapon set to: %s", weaponName.c_str());
            changed = true;
        }
    }

    if (!changed)
    {
        LOG_WARNING("No active players found");
    }
}

/**
 * Command: /boxes <1|0>
 *
 * Toggles bounding box debug visualization.
 * Shows collision boundaries for all entities (players, balls, shots, floors).
 */
void AppConsole::cmdBoxes(const std::string& args)
{
    if (args.empty())
    {
        LOG_WARNING("Usage: /boxes <1|0>");
        LOG_INFO("  1 = Enable bounding boxes, 0 = Disable bounding boxes");
        return;
    }

    // Get current scene
    AppData& appData = AppData::instance();
    Scene* scene = dynamic_cast<Scene*>(appData.currentScreen.get());
    if (!scene)
    {
        LOG_WARNING("Bounding boxes only available during gameplay");
        return;
    }

    // Parse argument
    int enable = std::atoi(args.c_str());

    if (enable != 0 && enable != 1)
    {
        LOG_ERROR("Invalid value: %s (must be 0 or 1)", args.c_str());
        return;
    }

    // Set the bounding box mode
    scene->setBoundingBoxes(enable == 1);

    if (enable)
    {
        LOG_SUCCESS("Bounding boxes enabled");
    }
    else
    {
        LOG_SUCCESS("Bounding boxes disabled");
    }
}

/**
 * /events command - Toggle event logging
 *
 * Enables or disables event system logging for debugging.
 * When enabled, all fired events are logged to the console.
 */
void AppConsole::cmdEvents(const std::string& args)
{
    // Toggle event logging
    bool currentState = EVENT_MGR.isLoggingEvents();
    bool newState = !currentState;

    EVENT_MGR.setLogEvents(newState);

    if (newState)
    {
        LOG_SUCCESS("Event logging enabled");
    }
    else
    {
        LOG_SUCCESS("Event logging disabled");
    }
}

/**
 * Command: /time <seconds>
 *
 * Sets the stage countdown timer to the specified value in seconds.
 * Only works during gameplay (Scene).
 */
void AppConsole::cmdTime(const std::string& args)
{
    if (args.empty())
    {
        LOG_WARNING("Usage: /time <seconds>");
        LOG_INFO("  Example: /time 60");
        return;
    }

    // Get current scene
    AppData& appData = AppData::instance();
    Scene* scene = dynamic_cast<Scene*>(appData.currentScreen.get());
    if (!scene)
    {
        LOG_WARNING("Time command only available during gameplay");
        return;
    }

    // Parse time value
    int newTime = 0;
    try
    {
        newTime = std::stoi(args);
    }
    catch (const std::exception&)
    {
        LOG_ERROR("Invalid time value: %s (must be a number)", args.c_str());
        return;
    }

    // Validate time range (0 to 999 seconds)
    if (newTime < 0 || newTime > 999)
    {
        LOG_WARNING("Time value %d is out of range (valid: 0-999 seconds)", newTime);
        return;
    }

    // Set the time
    scene->setTimeRemaining(newTime);
    LOG_SUCCESS("Stage time set to %d seconds", newTime);
}

/**
 * Command: /lives <player_num> <lives>
 *
 * Sets a specific player's life count.
 * Only works during gameplay (Scene).
 */
void AppConsole::cmdLives(const std::string& args)
{
    if (args.empty())
    {
        LOG_WARNING("Usage: /lives <player_num> <lives>");
        LOG_INFO("  player_num: 1 or 2");
        LOG_INFO("  lives: number of lives to set (0-99)");
        LOG_INFO("  Example: /lives 1 5");
        return;
    }

    // Get current scene
    AppData& appData = AppData::instance();
    Scene* scene = dynamic_cast<Scene*>(appData.currentScreen.get());
    if (!scene)
    {
        LOG_WARNING("Lives command only available during gameplay");
        return;
    }

    // Parse arguments
    std::istringstream iss(args);
    int playerNum = 0;
    int newLives = 0;
    iss >> playerNum >> newLives;

    // Validate player number
    if (playerNum < 1 || playerNum > 2)
    {
        LOG_ERROR("Invalid player number: %d (must be 1 or 2)", playerNum);
        return;
    }

    // Validate lives count
    if (newLives < 0 || newLives > 99)
    {
        LOG_ERROR("Invalid lives count: %d (must be 0-99)", newLives);
        return;
    }

    // Get the player
    Player* player = scene->getPlayer(playerNum - 1);  // Convert to 0-indexed
    if (!player)
    {
        LOG_WARNING("Player %d is not active", playerNum);
        return;
    }

    // Set the lives (we need to access the private member, so we'll use a friend or add a setter)
    // For now, let's add a public setter method to Player class
    player->setLives(newLives);
    LOG_SUCCESS("Player %d lives set to %d", playerNum, newLives);
}

/**
 * Command: /ball <x> <y> [size]
 *
 * Spawns a ball at the specified position during gameplay.
 * Use -1 for x or y to spawn at a random position.
 * Size is optional and defaults to 0 (largest ball).
 */
void AppConsole::cmdBall(const std::string& args)
{
    if (args.empty())
    {
        LOG_WARNING("Usage: /ball <x> <y> [size]");
        LOG_INFO("  x, y: Position (-1 for random)");
        LOG_INFO("  size: 0 (largest) to 3 (smallest), default 0");
        LOG_INFO("  Example: /ball 320 200 1");
        LOG_INFO("  Example: /ball -1 -1 0  (random position, largest ball)");
        return;
    }

    // Get current scene
    AppData& appData = AppData::instance();
    Scene* scene = dynamic_cast<Scene*>(appData.currentScreen.get());
    if (!scene)
    {
        LOG_WARNING("Ball command only available during gameplay");
        return;
    }

    // Parse arguments
    std::istringstream iss(args);
    int x = 0;
    int y = 0;
    int size = 0;  // Default to largest ball
    int count = 1; // Number of balls to spawn
    iss >> x >> y;

    // Size is optional
    if (!(iss >> size))
    {
        size = 0;  // Default to largest ball
    }
    else if (!(iss >> count))
    {
        count = 1;
    }

    // Validate size range (0-3)
    if (size < 0 || size > 3)
    {
        LOG_ERROR("Invalid ball size: %d (must be 0-3)", size);
        LOG_INFO("  0 = Largest (64px), 1 = Large (40px), 2 = Medium (24px), 3 = Smallest (16px)");
        return;
    }

    // Convert -1 to INT_MAX for random positioning (Scene's convention)
    if (x == -1) x = INT_MAX;
    if (y == -1) y = INT_MAX;

    // Spawn the ball with default physics parameters
    // Using defaults from addBall signature: top=0 (auto), dirX=1, dirY=1, id=0 (red ball)
    for(int i = 0; i < count; ++i)
        scene->addBall(x, y, size);

    const char* sizeNames[] = { "largest", "large", "medium", "smallest" };
    const char* sizeName = (size >= 0 && size <= 3) ? sizeNames[size] : "unknown";

    if (x == INT_MAX || y == INT_MAX)
    {
        LOG_SUCCESS("Spawning %s ball at random position", sizeName);
    }
    else
    {
        LOG_SUCCESS("Spawning %s ball at (%d, %d)", sizeName, x, y);
    }
}

/**
 * Command: /floor <x> <y> [type]
 *
 * Spawns a floor at the specified position during gameplay.
 * Type can be a string name or integer index matching FloorType enum order.
 */
void AppConsole::cmdFloor(const std::string& args)
{
    if (args.empty())
    {
        LOG_WARNING("Usage: /floor <x> <y> [type]");
        LOG_INFO("  type (string): vert_big, vert_middle, horiz_big, horiz_middle, small");
        LOG_INFO("  type (int):    0=vert_big 1=vert_middle 2=horiz_big 3=horiz_middle 4=small");
        LOG_INFO("  Default type: horiz_big");
        LOG_INFO("  Example: /floor 320 200 horiz_big");
        LOG_INFO("  Example: /floor 320 200 2   (same as horiz_big)");
        return;
    }

    // Get current scene
    AppData& appData = AppData::instance();
    Scene* scene = dynamic_cast<Scene*>(appData.currentScreen.get());
    if (!scene)
    {
        LOG_WARNING("Floor command only available during gameplay");
        return;
    }

    // Parse "x y [type]"
    std::istringstream iss(args);
    int x = 0, y = 0;
    std::string typeStr;
    iss >> x >> y >> typeStr;

    FloorType type = FloorType::HORIZ_BIG;  // default

    if (!typeStr.empty())
    {
        // Try string name first
        if      (typeStr == "vert_big")     type = FloorType::VERT_BIG;
        else if (typeStr == "vert_middle")  type = FloorType::VERT_MIDDLE;
        else if (typeStr == "horiz_big")    type = FloorType::HORIZ_BIG;
        else if (typeStr == "horiz_middle") type = FloorType::HORIZ_MIDDLE;
        else if (typeStr == "small")        type = FloorType::SMALL;
        else
        {
            // Try as integer (enum order 0–4)
            try
            {
                int typeInt = std::stoi(typeStr);
                if (typeInt >= 0 && typeInt <= 4)
                    type = static_cast<FloorType>(typeInt);
                else
                {
                    LOG_ERROR("Invalid floor type int: %d (must be 0-4)", typeInt);
                    return;
                }
            }
            catch (const std::exception&)
            {
                LOG_ERROR("Unknown floor type: %s", typeStr.c_str());
                LOG_INFO("  String: vert_big, vert_middle, horiz_big, horiz_middle, small");
                LOG_INFO("  Int:    0, 1, 2, 3, 4");
                return;
            }
        }
    }

    scene->addFloor(x, y, type);

    static const char* typeNames[] = { "vert_big", "vert_middle", "horiz_big", "horiz_middle", "small" };
    LOG_SUCCESS("Spawning %s floor at (%d, %d)", typeNames[static_cast<int>(type)], x, y);
}

/**
 * Command: /immune [player_num] [0|1]
 *
 * Toggles player immunity to ball collisions.
 * If player_num is omitted, applies to all active players.
 * If 0|1 is omitted, toggles current immunity state.
 */
void AppConsole::cmdImmune(const std::string& args)
{
    // Get current scene
    AppData& appData = AppData::instance();
    Scene* scene = dynamic_cast<Scene*>(appData.currentScreen.get());
    if (!scene)
    {
        LOG_WARNING("Immune command only available during gameplay");
        return;
    }

    // Parse arguments
    int playerNum = -1;  // -1 means all players
    int enable = -1;     // -1 means toggle

    if (!args.empty())
    {
        std::istringstream iss(args);
        iss >> playerNum;
        iss >> enable;
    }

    // Validate inputs
    if (playerNum < -1 || playerNum > 2 || playerNum == 0)
    {
        LOG_ERROR("Invalid player number: %d (must be 1, 2, or omit for all)", playerNum);
        return;
    }

    if (enable != -1 && enable != 0 && enable != 1)
    {
        LOG_ERROR("Invalid enable value: %d (must be 0, 1, or omit to toggle)", enable);
        return;
    }

    // Apply to specified player(s)
    bool changed = false;
    const int INFINITE_IMMUNITY = 999999;  // Very high value for permanent immunity

    auto togglePlayerImmunity = [&](Player* player, const char* playerName) {
        if (!player) return;

        if (enable == -1)
        {
            // Toggle: if immune, disable; if not immune, enable
            if (player->isImmune())
            {
                player->setImmuneCounter(0);
                LOG_SUCCESS("%s immunity disabled", playerName);
            }
            else
            {
                player->setImmuneCounter(INFINITE_IMMUNITY);
                LOG_SUCCESS("%s immunity enabled", playerName);
            }
        }
        else if (enable == 1)
        {
            player->setImmuneCounter(INFINITE_IMMUNITY);
            LOG_SUCCESS("%s immunity enabled", playerName);
        }
        else
        {
            player->setImmuneCounter(0);
            LOG_SUCCESS("%s immunity disabled", playerName);
        }
        changed = true;
    };

    if (playerNum == -1 || playerNum == 1)
    {
        Player* p1 = scene->getPlayer(0);
        togglePlayerImmunity(p1, "Player 1");
    }
    if (playerNum == -1 || playerNum == 2)
    {
        Player* p2 = scene->getPlayer(1);
        togglePlayerImmunity(p2, "Player 2");
    }

    if (!changed)
    {
        LOG_WARNING("No active players found");
    }
}

/**
 * Command: /shield [player_num] [0|1]
 *
 * Enables or disables the shield power-up for a player.
 * If player_num is omitted, applies to all active players.
 * If 0|1 is omitted, toggles current shield state.
 */
void AppConsole::cmdShield(const std::string& args)
{
    AppData& appData = AppData::instance();
    Scene* scene = dynamic_cast<Scene*>(appData.currentScreen.get());
    if (!scene)
    {
        LOG_WARNING("Shield command only available during gameplay");
        return;
    }

    int playerNum = -1;  // -1 means all players
    int enable = -1;     // -1 means toggle

    if (!args.empty())
    {
        std::istringstream iss(args);
        iss >> playerNum;
        iss >> enable;
    }

    if (playerNum < -1 || playerNum > 2 || playerNum == 0)
    {
        LOG_ERROR("Invalid player number: %d (must be 1, 2, or omit for all)", playerNum);
        return;
    }

    if (enable != -1 && enable != 0 && enable != 1)
    {
        LOG_ERROR("Invalid value: %d (must be 0, 1, or omit to toggle)", enable);
        return;
    }

    bool changed = false;

    auto applyShield = [&](Player* player, const char* playerName) {
        if (!player) return;

        bool newState = (enable == -1) ? !player->getShield() : (enable == 1);
        player->setShield(newState);
        LOG_SUCCESS("%s shield %s", playerName, newState ? "enabled" : "disabled");
        changed = true;
    };

    if (playerNum == -1 || playerNum == 1)
        applyShield(scene->getPlayer(0), "Player 1");
    if (playerNum == -1 || playerNum == 2)
        applyShield(scene->getPlayer(1), "Player 2");

    if (!changed)
        LOG_WARNING("No active players found");
}

void AppConsole::print(const std::string& message, LogColor color)
{
    // This bypasses Logger and adds directly to the display
    // For now, just use Logger
    LOG_INFO("%s", message.c_str());
}

void AppConsole::registerCommand(const std::string& name, const std::string& desc, CommandHandler handler)
{
    // Check if command already exists
    for (ConsoleCommand& cmd : commands)
    {
        if (cmd.name == name)
        {
            cmd.handler = handler;
            cmd.description = desc;
            return;
        }
    }
    
    ConsoleCommand cmd;
    cmd.name = name;
    cmd.description = desc;
    cmd.handler = handler;
    commands.push_back(cmd);
}

void AppConsole::unregisterCommand(const std::string& name)
{
    commands.erase(
        std::remove_if(commands.begin(), commands.end(),
            [&name](const ConsoleCommand& cmd) { return cmd.name == name; }),
        commands.end()
    );
}

bool AppConsole::handleEvent(const SDL_Event& event)
{
    // Toggle console with F9 (like Roblox) or backtick (`)
    if (event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.sym == SDLK_F9 ||
            event.key.keysym.sym == SDLK_BACKQUOTE)
        {
            toggle();
            LOG_DEBUG("AppConsole toggled: %s", visible ? "visible" : "hidden");
            return true;
        }
        
        if (visible)
        {
            handleKeyDown(event.key.keysym.sym, (SDL_Keymod)event.key.keysym.mod);
            return true;
        }
    }
    
    // Handle mouse wheel for scrolling (when console is visible)
    if (event.type == SDL_MOUSEWHEEL && visible)
    {
        if (event.wheel.y > 0)
        {
            // Scroll up
            scrollUp(3);
        }
        else if (event.wheel.y < 0)
        {
            // Scroll down
            scrollDown(3);
        }
        return true;
    }

    if (event.type == SDL_TEXTINPUT)
    {
        // Special case: "/" opens console with "/" pre-typed (if console is hidden and prompt is empty)
        if (!visible && event.text.text[0] == '/' && inputBuffer.empty())
        {
            show();
            inputBuffer = "/";
            cursorPosition = 1;
            LOG_DEBUG("AppConsole opened with '/' shortcut");
            return true;
        }

        // Normal text input when console is visible
        if (visible)
        {
            handleTextInput(event.text.text);
            return true;
        }
    }

    return false;
}

void AppConsole::handleTextInput(const char* text)
{
    inputBuffer.insert(cursorPosition, text);
    cursorPosition += strlen(text);
}

void AppConsole::handleKeyDown(SDL_Keycode key, SDL_Keymod mod)
{
    // Check for Ctrl+Up/Down for scrolling (like Minecraft/Roblox)
    bool ctrlPressed = (mod & KMOD_CTRL) != 0;
    
    switch (key)
    {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (!inputBuffer.empty())
            {
                processInput(inputBuffer);
                commandHistory.push_back(inputBuffer);
                historyIndex = -1;
                inputBuffer.clear();
                cursorPosition = 0;
                scrollToBottom(); // Auto-scroll to bottom on command execution
            }
            break;
            
        case SDLK_BACKSPACE:
            if (cursorPosition > 0)
            {
                inputBuffer.erase(cursorPosition - 1, 1);
                cursorPosition--;
            }
            break;
            
        case SDLK_DELETE:
            if (cursorPosition < inputBuffer.length())
            {
                inputBuffer.erase(cursorPosition, 1);
            }
            break;
            
        case SDLK_LEFT:
            if (cursorPosition > 0)
                cursorPosition--;
            break;
            
        case SDLK_RIGHT:
            if (cursorPosition < inputBuffer.length())
                cursorPosition++;
            break;
            
        case SDLK_HOME:
            if (ctrlPressed)
            {
                // Ctrl+Home: Scroll to top
                const std::vector<LogEntry>& entries = Logger::instance().getEntries();
                scrollOffset = std::max(0, (int)entries.size() - maxVisibleLines);
            }
            else
            {
                cursorPosition = 0;
            }
            break;
            
        case SDLK_END:
            if (ctrlPressed)
            {
                // Ctrl+End: Scroll to bottom
                scrollToBottom();
            }
            else
            {
                cursorPosition = inputBuffer.length();
            }
            break;
            
        case SDLK_UP:
            if (ctrlPressed)
            {
                // Ctrl+Up: Scroll up (like Minecraft)
                scrollUp(1);
            }
            else if (!commandHistory.empty())
            {
                // Normal Up: Command history
                if (historyIndex < 0)
                    historyIndex = (int)commandHistory.size() - 1;
                else if (historyIndex > 0)
                    historyIndex--;
                
                inputBuffer = commandHistory[historyIndex];
                cursorPosition = inputBuffer.length();
            }
            break;
            
        case SDLK_DOWN:
            if (ctrlPressed)
            {
                // Ctrl+Down: Scroll down (like Minecraft)
                scrollDown(1);
            }
            else if (!commandHistory.empty() && historyIndex >= 0)
            {
                // Normal Down: Command history
                historyIndex++;
                if (historyIndex >= (int)commandHistory.size())
                {
                    historyIndex = -1;
                    inputBuffer.clear();
                }
                else
                {
                    inputBuffer = commandHistory[historyIndex];
                }
                cursorPosition = inputBuffer.length();
            }
            break;
            
        case SDLK_PAGEUP:
            scrollUp(5);
            break;
            
        case SDLK_PAGEDOWN:
            scrollDown(5);
            break;
            
        case SDLK_ESCAPE:
            hide();
            break;
    }
}

void AppConsole::processInput(const std::string& input)
{
    if (input.empty())
        return;
    
    // Check if it's a command (starts with /)
    if (input[0] == '/')
    {
        executeCommand(input.substr(1));
    }
    else
    {
        // Treat as echo
        LOG_INFO("> %s", input.c_str());
    }
}

void AppConsole::executeCommand(const std::string& input)
{
    // Parse command and arguments
    std::istringstream iss(input);
    std::string cmdName;
    iss >> cmdName;
    
    // Get rest as arguments
    std::string args;
    std::getline(iss >> std::ws, args);
    
    // Convert command name to lowercase
    std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::tolower);
    
    // Find and execute command
    for (const ConsoleCommand& cmd : commands)
    {
        if (cmd.name == cmdName)
        {
            cmd.handler(args);
            return;
        }
    }
    
    LOG_WARNING("Unknown command: /%s (type /help for available commands)", cmdName.c_str());
}

void AppConsole::update()
{
    // Blink cursor
    Uint32 currentTime = SDL_GetTicks();
    if (currentTime - cursorBlinkTime > 500)
    {
        cursorVisible = !cursorVisible;
        cursorBlinkTime = currentTime;
    }
}

void AppConsole::render()
{
    if (!visible || !initialized || !graph)
        return;
    
    update();
    renderBackground();
    renderLogs();
    renderPrompt();
}

void AppConsole::renderBackground()
{
    SDL_Renderer* renderer = graph->getRenderer();
    
    // Use logical render dimensions, not window size
    const int screenW = RES_X;
    const int screenH = RES_Y;
    
    // Semi-transparent background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, (Uint8)(overlayAlpha * 255));
    
    SDL_Rect bgRect = { 0, 0, screenW, screenH };
    SDL_RenderFillRect(renderer, &bgRect);
    
    // Border line above prompt
    int promptY = screenH - promptHeight - padding;
    SDL_SetRenderDrawColor(renderer, 80, 80, 100, 200);
    SDL_RenderDrawLine(renderer, 0, promptY - 2, screenW, promptY - 2);
}

void AppConsole::renderLogs()
{
    if (!fontRenderer)
        return;
    
    const std::vector<LogEntry>& entries = Logger::instance().getEntries();
    if (entries.empty())
        return;
    
    SDL_Renderer* renderer = graph->getRenderer();
    
    // Use logical render dimensions, not window size
    const int screenW = RES_X;
    const int screenH = RES_Y;
    
    // Calculate visible area
    int logAreaHeight = screenH - promptHeight - padding * 3;
    int linesVisible = logAreaHeight / lineHeight;
    
    // Determine which entries to show
    int totalEntries = (int)entries.size();
    int startEntry = std::max(0, totalEntries - linesVisible - scrollOffset);
    int endEntry = std::min(totalEntries, startEntry + linesVisible);
    
    // Render entries
    int y = padding;
    for (int i = startEntry; i < endEntry; i++)
    {
        const LogEntry& entry = entries[i];
        
        // Set color
        fontRenderer->setColor(entry.color.r, entry.color.g, entry.color.b, 255);
        
        // Format: [HH:MM:SS] message
        char line[512];
        snprintf(line, sizeof(line), "[%s] %s", entry.timestamp.c_str(), entry.message.c_str());
        
        fontRenderer->text(line, padding, y);
        y += lineHeight;
    }
    
    // Reset color
    fontRenderer->setColor(255, 255, 255, 255);
    
    // Show scroll indicator if not at bottom
    if (scrollOffset > 0)
    {
        fontRenderer->setColor(150, 150, 150, 255);
        char scrollText[32];
        snprintf(scrollText, sizeof(scrollText), "-- %d more --", scrollOffset);
        
        int textW = fontRenderer->getTextWidth(scrollText);
        fontRenderer->text(scrollText, (screenW - textW) / 2, logAreaHeight - lineHeight);
        fontRenderer->setColor(255, 255, 255, 255);
    }
}

void AppConsole::renderPrompt()
{
    if (!fontRenderer)
        return;
    
    SDL_Renderer* renderer = graph->getRenderer();
    
    // Use logical render dimensions, not window size
    const int screenW = RES_X;
    const int screenH = RES_Y;
    
    int promptY = screenH - promptHeight - padding;
    
    // Prompt background
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 30, 30, 40, 220);
    SDL_Rect promptBg = { padding, promptY, screenW - padding * 2, promptHeight };
    SDL_RenderFillRect(renderer, &promptBg);
    
    // Prompt text
    fontRenderer->setColor(100, 200, 100, 255);
    fontRenderer->text("> ", padding + 4, promptY + 4);
    
    fontRenderer->setColor(255, 255, 255, 255);
    fontRenderer->text(inputBuffer.c_str(), padding + 20, promptY + 4);
    
    // Cursor
    if (cursorVisible)
    {
        std::string beforeCursor = inputBuffer.substr(0, cursorPosition);
        int cursorX = padding + 20 + fontRenderer->getTextWidth(beforeCursor.c_str());
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawLine(renderer, cursorX, promptY + 4, cursorX, promptY + promptHeight - 4);
    }
}

void AppConsole::scrollUp(int lines)
{
    const std::vector<LogEntry>& entries = Logger::instance().getEntries();
    int maxScroll = std::max(0, (int)entries.size() - maxVisibleLines);
    scrollOffset = std::min(scrollOffset + lines, maxScroll);
}

void AppConsole::scrollDown(int lines)
{
    scrollOffset = std::max(0, scrollOffset - lines);
}

void AppConsole::scrollToBottom()
{
    scrollOffset = 0;
}

bool AppConsole::queueQuickStageSwitchByOffset(int offset)
{
    AppData& appData = AppData::instance();

    Scene* currentScene = dynamic_cast<Scene*>(appData.currentScreen.get());
    if (!currentScene)
    {
        LOG_WARNING("Ultra-fast stage switch only available during gameplay");
        return false;
    }

    int targetStage = appData.currentStage + offset;
    if (targetStage < 1 || targetStage > appData.numStages)
    {
        LOG_WARNING("Stage %d is out of range (valid: 1-%d)", targetStage, appData.numStages);
        return false;
    }

    if (targetStage == appData.currentStage)
    {
        LOG_INFO("Already on level %d", targetStage);
        return false;
    }

    currentScene->queueQuickStageSwitch(targetStage);
    LOG_SUCCESS("Ultra-fast switch queued to level %d", targetStage);
    return true;
}
