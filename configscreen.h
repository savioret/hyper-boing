#pragma once

#include "app.h"
#include "graph.h"

/**
 * enum ConfigState
 */
enum class ConfigState
{
    Normal,      // Estado normal, navegando por opciones
    WaitingKey   // Esperando que el usuario pulse una tecla
};

/**
 * ConfigScreen class
 *
 * Key configuration screen using SDL2.
 * This is a child class of GameState and serves as a game module.
 * It replaces the Windows dialog with an SDL2 interface.
 */
class ConfigScreen : public GameState
{
private:
    ConfigState state;
    int selectedOption; // Opción seleccionada (0-6: 3 teclas por jugador + 1 modo pantalla)
    int waitingForKey;

    // Teclas temporales (se guardan al confirmar)
    SDL_Scancode tempKeys[2][3]; // [player][left/right/shoot]

    // Modo de renderizado temporal
    int tempRenderMode; // RENDERMODE_NORMAL o RENDERMODE_EXCLUSIVE
    Sprite fontBmp; // BMFont texture
public:
    ConfigScreen();
    virtual ~ConfigScreen() {}

    int init() override;
    void* moveAll() override;
    int drawAll() override;
    int release() override;

    void drawUI();
    void drawDebugOverlay() override;
    void drawText(const char* text, int x, int y, bool selected = false);
    void drawKeyName(SDL_Scancode key, int x, int y);
    void saveConfiguration();
    void cancelConfiguration();
    const char* getKeyName(SDL_Scancode scancode) const;

    // Getters
    ConfigState getState() const { return state; }
    int getSelectedOption() const { return selectedOption; }
};
