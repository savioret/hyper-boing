#include "pang.h"
#include "app.h"
#include "appdata.h"
#include <cstdio>
#include <iostream>
#include <cstring>

//--- Program Variable Declarations ------------------------------
int quit = 0;
bool goback = false;
int globalmode;
char WindowName[] = "Hyper Boing";
char WindowTitle[] = "¡Hyper Boing!";

GameState *screen = nullptr, *nextscreen = nullptr;

// Main simulation/game loop replacement for WinMain for cleaner look if requested, 
// but keeping WinMain as entry point for Windows.

int main(int argc, char* argv[])
{
    printf("Initializing....\n");
    
    // Get singleton instance
    AppData& appData = AppData::instance();
    
    // Load Configuration
    appData.config.load();

    // Initialize Input
    if (!appData.input.init())
    {
        printf("Error initializing Input\n");
        return 1;
    }
    
    // Initialize Graphics with SDL
    if (!appData.graph.init("Hyper Boing", globalmode))
    {
        printf("Error initializing Graphics\n");
        return 1;
    }

    appData.init();
    screen = new Menu();
    screen->init();

    SDL_Event e;
    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) { quit = 1; }
            if (e.type == SDL_KEYDOWN)
            {
                 if (e.key.keysym.sym == SDLK_ESCAPE)
                 {
                     if (!appData.isMenu())
                        goback = true;
                 }
                 if (e.key.keysym.sym == SDLK_p)
                 {
                     screen->setPause(!screen->isPaused());
                 }
                 if (e.key.keysym.sym == SDLK_c)
                 {
                     nextscreen = new ConfigScreen();
                     if (nextscreen) {
                         screen->release();
                         delete screen;
                         screen = nextscreen;
                         screen->init();
                     }
                 }
                 if (e.key.keysym.sym == SDLK_TAB)
                 {
                     appData.debugMode = !appData.debugMode;
                 }
            }
        }

        if (quit) break;

        if (screen->isPaused())
            screen->doPause();
        else
        {
            nextscreen = (GameState*)screen->doTick();
            if (nextscreen != nullptr)
            {
                screen->release();
                delete screen;
                screen = nextscreen;
                appData.setCurrent(screen);
                screen->init();
            }
        }
    }

    appData.config.save();
    appData.graph.release();
    CloseMusic();
    
    AppData::destroy();

    return 0;
}

// Windows entry point wrapper
#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    return main(__argc, __argv);
}
#endif

// Windows MCI Sound wrappers - Note: these remain platform specific
#ifdef _WIN32
#include <windows.h>
char OpenMusic(char* filename)
{
    char buffer[256];
    std::sprintf(buffer, "open %s type sequencer alias MUSIC", filename);
    return (mciSendStringA(buffer, nullptr, 0, nullptr) == 0);
}

char PlayMusic()
{
    return (mciSendStringA("play MUSIC from 0 notify", nullptr, 0, nullptr) == 0);
}

char StopMusic()
{
    return (mciSendStringA("stop MUSIC", nullptr, 0, nullptr) == 0);
}

char ContinueMusic()
{
    return (mciSendStringA("play MUSIC notify", nullptr, 0, nullptr) == 0);
}
 
char CloseMusic()
{
    return (mciSendStringA("close all", nullptr, 0, nullptr) == 0);
}
#else
char OpenMusic(char*) { return 0; }
char PlayMusic() { return 0; }
char StopMusic() { return 0; }
char ContinueMusic() { return 0; }
char CloseMusic() { return 0; }
#endif