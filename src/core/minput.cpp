#include <SDL.h>
#include "minput.h"
#include "appconsole.h"

MInput::MInput()
{
}

MInput::~MInput()
{
}

bool MInput::init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        return false;

    return true;
}

bool MInput::key(SDL_Scancode k)
{
    // Block all game input when AppConsole is visible
    if (AppConsole::instance().isVisible())
    {
        return false;
    }
    
    const Uint8* keyState = SDL_GetKeyboardState(nullptr);
    return keyState[k] != 0;
}

bool MInput::reacquireInput()
{
    return true;
}