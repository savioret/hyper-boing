#pragma once

#include "appdata.h"

/**
 * GameRunner class
 *
 * Application lifecycle manager that encapsulates the main game loop.
 * Handles initialization, event processing, state transitions, and cleanup.
 *
 * Responsibilities:
 * - Initialize subsystems (Graphics, Input, Audio, Configuration)
 * - Execute the main game loop with proper timing
 * - Process SDL events and input
 * - Manage state transitions between screens (Menu, Scene, Config)
 * - Clean shutdown of all resources
 *
 * Design: Simple, non-singleton class that can be instantiated from main()
 */
class GameRunner
{
private:
    AppData& appData;
    bool isInitialized;
    
    // Lifecycle methods
    bool initialize();
    void processEvents();
    void update();
    void handleStateTransition();
    void shutdown();

public:
    GameRunner();
    ~GameRunner();
    
    // Main entry point - returns exit code
    int run();
};
