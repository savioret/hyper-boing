#ifndef CONFIGSCREEN_H_
#define CONFIGSCREEN_H_

#include "app.h"
#include "graph.h"

/********************************************************
  clase PCONFIGSCREEN

  Pantalla de configuración de teclas usando SDL2.
  Reemplaza el diálogo de Windows con una interfaz SDL2.
********************************************************/

enum ConfigState
{
    CONFIG_NORMAL,      // Estado normal, navegando por opciones
    CONFIG_WAITING_KEY  // Esperando que el usuario pulse una tecla
};

class PCONFIGSCREEN : public PAPP
{
public:
    // Estado de la configuración
    ConfigState state;
    int selectedOption; // Opción seleccionada (0-6: 3 teclas por jugador + 1 modo pantalla)
    int waitingForKey;  // Qué tecla estamos esperando configurar

    // Teclas temporales (se guardan al confirmar)
    SDL_Scancode tempKeys[2][3]; // [player][left/right/shoot]

    // Modo de renderizado temporal
    int tempRenderMode; // RENDERMODE_NORMAL o RENDERMODE_EXCLUSIVE

    PCONFIGSCREEN ();

    int Init ();
    void* MoveAll ();
    int DrawAll ();
    int Release ();

    void DrawUI ();
    void DrawText ( const char* text, int x, int y, bool selected = false );
    void DrawKeyName ( SDL_Scancode key, int x, int y );
    void SaveConfiguration ();
    void CancelConfiguration ();
    const char* GetKeyName ( SDL_Scancode scancode );
};

#endif
