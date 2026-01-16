#include "gamerunner.h"
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * Main entry point for the application
 */
int main(int argc, char* argv[])
{
#ifdef _WIN32
#ifdef _DEBUG
    // Allocate a console for Debug builds
    AllocConsole();
    
    // Redirect stdout to console
    FILE* fpStdout = nullptr;
    freopen_s(&fpStdout, "CONOUT$", "w", stdout);
    
    // Redirect stderr to console
    FILE* fpStderr = nullptr;
    freopen_s(&fpStderr, "CONOUT$", "w", stderr);
    
    // Set console title
    SetConsoleTitle(TEXT("Hyper Boing - Debug Console"));
    
    printf("===========================================\n");
    printf("  HYPER BOING - DEBUG MODE\n");
    printf("===========================================\n");
    printf("Console output enabled for debugging\n\n");
#endif
#endif
    
    GameRunner runner;
    int result = runner.run();
    
#ifdef _WIN32
#ifdef _DEBUG
    printf("\n===========================================\n");
    printf("Game exited with code: %d\n", result);
    printf("Press any key to close console...\n");
    getchar();
    
    // Cleanup
    if (fpStdout) fclose(fpStdout);
    if (fpStderr) fclose(fpStderr);
    FreeConsole();
#endif
#endif
    
    return result;
}

/**
 * Windows entry point wrapper
 */
#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif