#pragma once

#include <SDL.h>

/**
 * MInput class
 *
 * Class that initializes the SDL2 library and 
 * performs keyboard buffer queries.
 */
class MInput
{
public:
    MInput();
    ~MInput();

    bool init();
    bool key(SDL_Scancode k);
    bool reacquireInput();
};

