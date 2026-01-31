#pragma once

#include "app.h"
#include "graph.h"

/**
 * enum ConfigState
 */
enum class ConfigState
{
    Normal,         // Estado normal, navegando por opciones
    EnteringEdit,   // Just entered edit mode, waiting for ENTER to be released
    WaitingKey      // Esperando que el usuario pulse una tecla
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
    
    // Input state tracking (previously static in moveAll)
    bool upPressed;
    bool downPressed;
    bool leftPressed;
    bool rightPressed;
    bool enterPressed;
    bool escapePressed;
    
    // Edit mode delay to prevent capturing ENTER key
    float editModeDelay;
    static constexpr float EDIT_MODE_DELAY_TIME = 0.2f; // 200ms delay
    
    // Initial screen delay to prevent capturing ENTER from menu navigation
    float initialDelay;
    static constexpr float INITIAL_DELAY_TIME = 0.2f; // delay on screen entry
    
    // Post-configuration delay to prevent capturing navigation keys that were just bound
    float postConfigDelay;
    static constexpr float POST_CONFIG_DELAY_TIME = 0.2f; // delay after key config
    
    // Custom modal overlay for key configuration (separate from debug overlay)
    TextOverlay configModalOverlay;
    
    void drawConfigModal();
    
public:
    ConfigScreen();
    virtual ~ConfigScreen() {}

    int init() override;
    GameState* moveAll(float dt) override;
    int drawAll() override;
    int release() override;

    void drawUI();
    void drawDebugOverlay() override;
    void drawText(const char* text, int x, int y, bool selected = false);
    void drawKeyName(SDL_Scancode key, int x, int y);
    void saveConfiguration();
    void cancelConfiguration();

    // Getters
    ConfigState getState() const { return state; }
    int getSelectedOption() const { return selectedOption; }
};
