#ifndef MINPUT_H_
#define MINPUT_H_

#include <SDL.h>

#define KEYDOWN(name,key) (name[key] & 0x80)  



/********************************************************
 clase MINPUT

  clase que inicializa la libreria SDL2
  y realiza consultas del buffer del teclado.
********************************************************/

class MINPUT
{
    //Uint8 buffer[SDL_NUM_SCANCODES];
   // SDL_Keyboard* sdlKeyboard; // SDL keyboard device

public:
    MINPUT ();
    ~MINPUT ();
    BOOL Init ( HINSTANCE hInst, HWND hwnd ); // Assuming you won't need these parameters for SDL2
    //BOOL ReadKeyboard ();
    BOOL Key ( SDL_Scancode k );
    BOOL ReacquireInput ( void );
};
#endif

