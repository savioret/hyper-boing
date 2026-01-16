#include "gamerunner.h"
#include "pang.h"
#include "menu.h"
#include "configscreen.h"
#include <cstdio>

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
    printf("Initializing game...\n");
    
    // Load configuration
    appData.config.load();
    
    // Initialize input subsystem
    if (!appData.input.init())
    {
        printf("Error: Failed to initialize input subsystem\n");
        return false;
    }
    
    // Initialize graphics subsystem
    if (!appData.graph.init("Hyper Boing", appData.renderMode))
    {
        printf("Error: Failed to initialize graphics subsystem\n");
        return false;
    }
    
    // Initialize audio subsystem (SDL_mixer)
    if (!AudioManager::instance().init())
    {
        printf("Error: Failed to initialize audio subsystem\n");
        return false;
    }
    
    // Initialize game data (player sprites, etc.)
    appData.init();
    
    // Preload menu music to avoid delays when returning from other screens
    printf("Preloading menu music...\n");
    appData.preloadMenuMusic();
    
    // Create and initialize first screen (Menu)
    appData.currentScreen = new Menu();
    appData.currentScreen->init();
    
    isInitialized = true;
    printf("Initialization complete!\n");
    
    return true;
}

void GameRunner::processEvents()
{
    SDL_Event e;
    
    while (SDL_PollEvent(&e))
    {
        // Handle window close event
        if (e.type == SDL_QUIT)
        {
            appData.quit = true;
        }
        
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
                    appData.nextScreen = new ConfigScreen();
                    if (appData.nextScreen)
                    {
                        handleStateTransition();
                    }
                    break;
                    
                case SDLK_TAB:
                    // TAB key - toggle debug mode
                    appData.debugMode = !appData.debugMode;
                    break;
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
    appData.nextScreen = static_cast<GameState*>(appData.currentScreen->doTick());
    
    // Check if state transition is needed
    if (appData.nextScreen != nullptr)
    {
        handleStateTransition();
    }
}

void GameRunner::handleStateTransition()
{
    if (!appData.nextScreen || !appData.currentScreen)
        return;
    
    // Cleanup old screen
    appData.currentScreen->release();
    delete appData.currentScreen;
    
    // Switch to new screen
    appData.currentScreen = appData.nextScreen;
    appData.nextScreen = nullptr;
    
    // Initialize new screen
    appData.setCurrent(appData.currentScreen);
    appData.currentScreen->init();
}

void GameRunner::shutdown()
{
    printf("Shutting down...\n");
    
    // Save configuration
    appData.config.save();
    
    // Release graphics subsystem
    appData.graph.release();
    
    // Destroy singletons
    AudioManager::destroy();
    AppData::destroy();
    
    isInitialized = false;
    printf("Shutdown complete\n");
}

int GameRunner::run()
{
    // Initialize all subsystems
    if (!initialize())
    {
        printf("Failed to initialize game\n");
        return 1;
    }
    
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
