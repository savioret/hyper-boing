#include "gamerunner.h"
#include "pang.h"
#include "menu.h"
#include "configscreen.h"
#include "logger.h"
#include "appconsole.h"
#include <cstdlib>
#include <ctime>

GameRunner::GameRunner()
    : appData(AppData::instance()), isInitialized(false)
{
}

GameRunner::~GameRunner()
{
    if (isInitialized)
    {
        shutdown();
    }
}

bool GameRunner::initialize()
{
    LOG_INFO("Initializing game...");
    
    // Initialize random number generator with current time as seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    LOG_DEBUG("Random number generator initialized");
    
    // Load configuration
    appData.config.load();
    LOG_DEBUG("Configuration loaded");
    
    // Initialize input subsystem
    if (!appData.input.init())
    {
        LOG_ERROR("Failed to initialize input subsystem");
        return false;
    }
    LOG_SUCCESS("Input subsystem initialized");
    
    // Initialize graphics subsystem
    if (!appData.graph.init("Hyper Boing", appData.renderMode))
    {
        LOG_ERROR("Failed to initialize graphics subsystem");
        return false;
    }
    LOG_SUCCESS("Graphics subsystem initialized");
    
    // Initialize audio subsystem (SDL_mixer)
    if (!AudioManager::instance().init())
    {
        LOG_ERROR("Failed to initialize audio subsystem");
        return false;
    }
    LOG_SUCCESS("Audio subsystem initialized");
    
    // Initialize in-game console
    if (!AppConsole::instance().init(&appData.graph))
    {
        LOG_WARNING("Failed to initialize AppConsole (non-critical)");
    }
    else
    {
        LOG_SUCCESS("AppConsole initialized");
    }
    
    // Initialize game data (player sprites, etc.)
    appData.init();
    LOG_DEBUG("Game data initialized");
    
    // Preload menu music to avoid delays when returning from other screens
    LOG_INFO("Preloading menu music...");
    appData.preloadMenuMusic();
    
    // Create and initialize first screen (Menu)
    appData.currentScreen = std::make_unique<Menu>();
    appData.currentScreen->init();
    LOG_DEBUG("Menu screen created");
    
    isInitialized = true;
    LOG_SUCCESS("Initialization complete!");
    
    return true;
}

void GameRunner::processEvents()
{
    SDL_Event e;
    
    while (SDL_PollEvent(&e))
    {
        // Let AppConsole handle events first (if visible, it consumes input)
        if (AppConsole::instance().handleEvent(e))
        {
            continue; // Console consumed the event, don't process game input
        }
        
        // Handle window close event
        if (e.type == SDL_QUIT)
        {
            appData.quit = true;
        }
        
        // Only process game events if console is NOT visible
        if (!AppConsole::instance().isVisible())
        {
            // Handle keyboard events
            if (e.type == SDL_KEYDOWN)
            {
                switch (e.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        // ESC key - return to menu if not already there
                        if (!appData.isMenu())
                        {
                            appData.goBack = true;
                        }
                        break;
                        
                    case SDLK_p:
                        // P key - toggle pause
                        if (appData.currentScreen)
                        {
                            appData.currentScreen->setPause(!appData.currentScreen->isPaused());
                        }
                        break;
                        
                    case SDLK_c:
                        // C key - open config screen
                        appData.nextScreen = std::make_unique<ConfigScreen>();
                        if (appData.nextScreen)
                        {
                            handleStateTransition();
                        }
                        break;
                        
                    case SDLK_TAB:
                        // TAB key - toggle debug mode
                        appData.debugMode = !appData.debugMode;
                        LOG_DEBUG("Debug mode: %s", appData.debugMode ? "ON" : "OFF");
                        break;
                }
            }
        }
    }
}

void GameRunner::update()
{
    if (!appData.currentScreen)
        return;
    
    // Handle paused state
    if (appData.currentScreen->isPaused())
    {
        appData.currentScreen->doPause();
        return;
    }
    
    // Execute current screen logic (update + render)
    // NOTE: AppConsole is now rendered inside each screen's drawAll() before flip()
    GameState* newScreen = appData.currentScreen->doTick();
    
    // Check if state transition is needed
    if (newScreen != nullptr)
    {
        appData.nextScreen.reset(newScreen);
        handleStateTransition();
    }
}

void GameRunner::handleStateTransition()
{
    if (!appData.nextScreen || !appData.currentScreen)
        return;
    
    LOG_DEBUG("State transition: switching screens");
    
    // Cleanup old screen
    appData.currentScreen->release();
    
    // Switch to new screen (unique_ptr automatically deletes old screen)
    appData.currentScreen = std::move(appData.nextScreen);
    
    // Initialize new screen
    appData.setCurrent(appData.currentScreen.get());
    appData.currentScreen->init();
}

void GameRunner::shutdown()
{
    LOG_INFO("Shutting down...");
    
    // Save configuration
    appData.config.save();
    LOG_DEBUG("Configuration saved");
    
    // Release AppConsole
    AppConsole::destroy();
    LOG_DEBUG("AppConsole released");
    
    // Release graphics subsystem
    appData.graph.release();
    
    // Destroy singletons
    AudioManager::destroy();
    LOG_DEBUG("AudioManager released");
    
    AppData::destroy();
    
    isInitialized = false;
    LOG_SUCCESS("Shutdown complete");
}

int GameRunner::run()
{
    // Initialize all subsystems
    if (!initialize())
    {
        LOG_ERROR("Failed to initialize game");
        return 1;
    }
    
    LOG_INFO("Entering main game loop");
    LOG_INFO("Press F9 or ` (backtick) to toggle in-game console");
    LOG_INFO("Press TAB to toggle debug overlay");
    
    // Main game loop
    while (!appData.quit)
    {
        processEvents();
        update();
    }
    
    // Cleanup
    shutdown();
    
    return 0;
}
