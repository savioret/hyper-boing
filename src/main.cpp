#include "gamerunner.h"
#include "logger.h"

/**
 * Main entry point for the application
 */
int main(int argc, char* argv[])
{
    // Initialize logger with console in Debug builds
#ifdef _DEBUG
    Logger::instance().init(true, LogLevel::TRACE);
    LOG_INFO ("Starting Hyper Boing (Debug Version)...");
#else
    Logger::instance().init(false, LogLevel::INFO);
    LOG_INFO("Starting Hyper Boing...");
#endif
    
    
    GameRunner runner;
    int result = runner.run();
    
    LOG_INFO("Game exited with code: %d", result);
    
    // Cleanup logger (will wait for keypress in Debug)
    Logger::destroy();
    
    return result;
}

/**
 * Windows entry point wrapper
 */
#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif