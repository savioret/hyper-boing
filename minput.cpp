#include <SDL.h>
#include "pang.h"

MINPUT::MINPUT ()
{
    // Initialize buffer
    //memset ( buffer, 0, sizeof ( buffer ) );
}

MINPUT::~MINPUT ()
{
    // Release SDL input device
    //if ( sdlKeyboard )
    //    SDL_DestroyKeyboard ( sdlKeyboard );
}

BOOL MINPUT::Init ( HINSTANCE hInst, HWND hwnd )
{
    // Initialize SDL
    if ( SDL_Init ( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
        return FALSE;

    // Create SDL window (if needed)
    // HWND hwnd could be used to create an SDL window if required.

    // Create SDL keyboard device
    //sdlKeyboard = SDL_CreateKeyboard ();

    //return (sdlKeyboard != nullptr);
    return true;
}

//BOOL MINPUT::ReadKeyboard ()
//{
//    // Update keyboard state
//    SDL_PumpEvents ();
//    int numKeys;
//    const Uint8* keyState = SDL_GetKeyboardState ( &numKeys );
//
//    // Copy SDL keyboard state to buffer
//    memcpy ( buffer, keyState, sizeof ( int ) * numKeys );
//
//    return TRUE;
//}

BOOL MINPUT::Key ( SDL_Scancode k )
{
    const Uint8* keyState = SDL_GetKeyboardState ( NULL );
    // Check if key k is pressed
    return keyState[k];
}

BOOL MINPUT::ReacquireInput ( void )
{
    // SDL does not require explicit acquisition or reacquisition
    return TRUE;
}