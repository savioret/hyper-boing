#include "pang.h"
#include "configscreen.h"
#include <SDL.h>

extern GRAPH graph;
extern MINPUT input;
extern PGAMEINFO gameinf;
extern int globalmode;

PCONFIGSCREEN::PCONFIGSCREEN()
{
    state = CONFIG_NORMAL;
    selectedOption = 0;
    waitingForKey = -1;
}

int PCONFIGSCREEN::Init()
{
    // Llamar a la inicialización base
    PAPP::Init();
    
    // Inicializar el fondo compartido si no está ya
    PAPP::InitSharedBackground();
    
    // Copiar las teclas actuales a las temporales
    for (int player = 0; player < 2; player++)
    {
        tempKeys[player][0] = gameinf.keys[player].left;
        tempKeys[player][1] = gameinf.keys[player].right;
        tempKeys[player][2] = gameinf.keys[player].shoot;
    }
    
    // Copiar el modo de renderizado actual
    tempRenderMode = globalmode;
    
    return 1;
}

void * PCONFIGSCREEN::MoveAll()
{
    // Actualizar el fondo scrolling
    PAPP::UpdateScrollingBackground();
    
    if (state == CONFIG_WAITING_KEY)
    {
        // Esperando a que el usuario pulse una tecla
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        
        // Buscar cualquier tecla pulsada
        for (int i = SDL_SCANCODE_A; i < SDL_NUM_SCANCODES; i++)
        {
            SDL_Scancode scancode = static_cast<SDL_Scancode>(i);
            if (keystate[scancode])
            {
                // Asignar la tecla
                int player = selectedOption / 3;
                int keyIndex = selectedOption % 3;
                tempKeys[player][keyIndex] = scancode;
                
                state = CONFIG_NORMAL;
                
                // Esperar a que se suelte la tecla
                SDL_Delay(200);
                break;
            }
        }
        
        // ESC para cancelar la espera
        if (input.Key(SDL_SCANCODE_ESCAPE))
        {
            state = CONFIG_NORMAL;
            SDL_Delay(200);
        }
    }
    else // CONFIG_NORMAL
    {
        // Navegación con teclas
        static bool upPressed = false;
        static bool downPressed = false;
        static bool leftPressed = false;
        static bool rightPressed = false;
        
        if (input.Key(SDL_SCANCODE_UP))
        {
            if (!upPressed)
            {
                selectedOption--;
                if (selectedOption < 0)
                    selectedOption = 6; // 7 opciones (0-6)
                upPressed = true;
            }
        }
        else
        {
            upPressed = false;
        }
        
        if (input.Key(SDL_SCANCODE_DOWN))
        {
            if (!downPressed)
            {
                selectedOption++;
                if (selectedOption > 6)
                    selectedOption = 0;
                downPressed = true;
            }
        }
        else
        {
            downPressed = false;
        }
        
        // Enter para cambiar la tecla seleccionada (excepto en modo pantalla)
        if (input.Key(SDL_SCANCODE_RETURN))
        {
            if(selectedOption < 6) // Solo para las teclas, no para el modo
            {
                state = CONFIG_WAITING_KEY;
                waitingForKey = selectedOption;
                SDL_Delay(200);
            }
        }
        
        // Flechas izquierda/derecha para cambiar el modo de pantalla
        if(selectedOption == 6) // Modo de pantalla
        {
            if (input.Key(SDL_SCANCODE_LEFT))
            {
                if (!leftPressed)
                {
                    tempRenderMode = RENDERMODE_NORMAL;
                    leftPressed = true;
                }
            }
            else
                leftPressed = false;
                
            if (input.Key(SDL_SCANCODE_RIGHT))
            {
                if (!rightPressed)
                {
                    tempRenderMode = RENDERMODE_EXCLUSIVE;
                    rightPressed = true;
                }
            }
            else
                rightPressed = false;
        }
        
        // F1 para guardar y salir
        if (input.Key(SDL_SCANCODE_F1))
        {
            SaveConfiguration();
            return new PMENU();
        }
        
        // ESC para cancelar y volver al menú
        if (input.Key(SDL_SCANCODE_ESCAPE))
        {
            CancelConfiguration();
            return new PMENU();
        }
    }
    
    return NULL;
}

int PCONFIGSCREEN::DrawAll()
{
    // Establecer el render target al backbuffer
    //SDL_SetRenderTarget(graph.renderer, graph.backBuffer);
    
    // Dibujar el fondo scrolling compartido
    PAPP::DrawScrollingBackground();
    
    // Dibujar la interfaz de configuración encima
    DrawUI();
    
    graph.Flip();
    
    return 1;
}

void PCONFIGSCREEN::DrawUI()
{
    int y = 60;
    int xLabel = 80;
    int xKey = 320;
    int lineHeight = 35;
    
    // Título
    DrawText("CONFIGURACION", 220, 20, false);
    
    // Separador
    SDL_SetRenderDrawColor(graph.renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(graph.renderer, 60, 50, 580, 50);
    
    // Jugador 1
    DrawText("JUGADOR 1:", xLabel, y, false);
    y += lineHeight;
    
    DrawText("Izquierda:", xLabel + 20, y, selectedOption == 0);
    DrawKeyName(tempKeys[0][0], xKey, y);
    y += lineHeight;
    
    DrawText("Derecha:", xLabel + 20, y, selectedOption == 1);
    DrawKeyName(tempKeys[0][1], xKey, y);
    y += lineHeight;
    
    DrawText("Disparar:", xLabel + 20, y, selectedOption == 2);
    DrawKeyName(tempKeys[0][2], xKey, y);
    y += lineHeight + 15;
    
    // Jugador 2
    DrawText("JUGADOR 2:", xLabel, y, false);
    y += lineHeight;
    
    DrawText("Izquierda:", xLabel + 20, y, selectedOption == 3);
    DrawKeyName(tempKeys[1][0], xKey, y);
    y += lineHeight;
    
    DrawText("Derecha:", xLabel + 20, y, selectedOption == 4);
    DrawKeyName(tempKeys[1][1], xKey, y);
    y += lineHeight;
    
    DrawText("Disparar:", xLabel + 20, y, selectedOption == 5);
    DrawKeyName(tempKeys[1][2], xKey, y);
    y += lineHeight + 15;
    
    // Separador
    SDL_SetRenderDrawColor(graph.renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(graph.renderer, 60, y, 580, y);
    y += 15;
    
    // Modo de pantalla
    DrawText("Modo de Pantalla:", xLabel, y, selectedOption == 6);
    const char* modeText = (tempRenderMode == RENDERMODE_NORMAL) ? "Ventana" : "Pantalla Completa";
    DrawText(modeText, xKey, y, false);
    y += lineHeight + 15;
    
    // Instrucciones
    SDL_SetRenderDrawColor(graph.renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(graph.renderer, 60, y, 580, y);
    y += 15;
    
    if (state == CONFIG_WAITING_KEY)
    {
        DrawText("Pulsa una tecla... (ESC para cancelar)", 120, y, false);
    }
    else
    {
        DrawText("Flechas: Navegar  |  ENTER: Cambiar tecla", 80, y, false);
        DrawText("F1: Guardar  |  ESC: Cancelar", 160, y + 20, false);
    }
}

void PCONFIGSCREEN::DrawText(const char* text, int x, int y, bool selected)
{
    if (selected)
    {
        // Dibujar indicador de selección
        SDL_SetRenderDrawColor(graph.renderer, 255, 255, 0, 255);
        SDL_Rect selRect = {x - 10, y, 5, 16};
        SDL_RenderFillRect(graph.renderer, &selRect);
    }
    
    // Usar graph.Text si está disponible
    graph.Text(text, x, y);
}

void PCONFIGSCREEN::DrawKeyName(SDL_Scancode key, int x, int y)
{
    const char* keyName = GetKeyName(key);
    graph.Text(keyName, x, y);
}

const char* PCONFIGSCREEN::GetKeyName(SDL_Scancode scancode)
{
    const char* name = SDL_GetScancodeName(scancode);
    return name ? name : "Unknown";
}

void PCONFIGSCREEN::SaveConfiguration()
{
    // Guardar las teclas configuradas
    for (int player = 0; player < 2; player++)
    {
        gameinf.keys[player].SetLeft(tempKeys[player][0]);
        gameinf.keys[player].SetRight(tempKeys[player][1]);
        gameinf.keys[player].SetShoot(tempKeys[player][2]);
    }
    
    // Guardar el modo de renderizado
    globalmode = tempRenderMode;
}

void PCONFIGSCREEN::CancelConfiguration()
{
    // No hacer nada, las configuraciones temporales se descartan
}

int PCONFIGSCREEN::Release()
{
    delete this;
    return 1;
}
