#include "pang.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>

extern int quit;

int PMENU::InitBitmaps()
{
    bmp.title.Init(&graph, "graph\\title.png", 0, 0);
    graph.SetColorKey(bmp.title.bmp, RGB(0,255,0));
    
    // Inicializar el fondo compartido
    PAPP::InitSharedBackground();
    
    // Cargar las nuevas opciones del menú (si no existen, se manejarán como NULL)
    bmp.optionPlay.Init(&graph, "graph\\menu_jugar.png", 0, 0);
    if(bmp.optionPlay.bmp)
        graph.SetColorKey(bmp.optionPlay.bmp, RGB(255,0,255));
        
    bmp.optionConfig.Init(&graph, "graph\\menu_config.png", 0, 0);
    if(bmp.optionConfig.bmp)
        graph.SetColorKey(bmp.optionConfig.bmp, RGB(255,0,255));
        
    bmp.optionExit.Init(&graph, "graph\\menu_salir.png", 0, 0);
    if(bmp.optionExit.bmp)
        graph.SetColorKey(bmp.optionExit.bmp, RGB(255,0,255));

    return 1;
}


int PMENU::Init()
{
    // Llamar a la inicialización base
    PAPP::Init();
    
    gameinf.menu = TRUE;
    InitBitmaps();
    x = 180; // Centrar mejor el título
    y = -300; // Empezar más arriba

    selectedOption = 0; // Empezar en JUGAR
    
    visible = FALSE;
    cont = 30;

    CloseMusic();
    OpenMusic("music\\menu.mid");
    PlayMusic();

    return 1;
}

int PMENU::Release()
{
    bmp.title.Release();
    bmp.optionPlay.Release();
    bmp.optionConfig.Release();
    bmp.optionExit.Release();

    CloseMusic();
    
    delete this;

    return 1;
}

void PMENU::DrawTitle()
{
    // Centrar el título horizontalmente
    int titleX = (RES_X - bmp.title.sx) / 2;
    int titleY = y;
    
    graph.DrawClipped(&bmp.title, titleX, titleY);
}

void PMENU::DrawMenu()
{
    // Calcular posiciones centradas
    int menuStartY = 320; // Empezar las opciones más abajo
    int spacing = 50;
    
    // Centrar las opciones horizontalmente
    int option1X = (RES_X - (bmp.optionPlay.bmp ? bmp.optionPlay.sx : 150)) / 2;
    int option2X = (RES_X - (bmp.optionConfig.bmp ? bmp.optionConfig.sx : 200)) / 2;
    int option3X = (RES_X - (bmp.optionExit.bmp ? bmp.optionExit.sx : 150)) / 2;
    
    // OPCIÓN 1: JUGAR
    if (bmp.optionPlay.bmp)
    {
        if (selectedOption == 0 && visible)
            graph.Draw(&bmp.optionPlay, option1X, menuStartY);
        else if(selectedOption != 0)
            graph.Draw(&bmp.optionPlay, option1X, menuStartY);
    }
    else
    {
        if (selectedOption != 0 || visible)
            graph.Text("JUGAR", option1X, menuStartY);
    }
    
    // OPCIÓN 2: CONFIGURACIÓN
    if (bmp.optionConfig.bmp)
    {
        if (selectedOption == 1 && visible)
            graph.Draw(&bmp.optionConfig, option2X, menuStartY + spacing);
        else if(selectedOption != 1)
            graph.Draw(&bmp.optionConfig, option2X, menuStartY + spacing);
    }
    else
    {
        if (selectedOption != 1 || visible)
            graph.Text("CONFIGURACION", option2X, menuStartY + spacing);
    }
        
    // OPCIÓN 3: SALIR
    if (bmp.optionExit.bmp)
    {
        if (selectedOption == 2 && visible)
         graph.Draw(&bmp.optionExit, option3X, menuStartY + spacing * 2);
        else if(selectedOption != 2)
            graph.Draw(&bmp.optionExit, option3X, menuStartY + spacing * 2);
    }
    else
    {
        if (selectedOption != 2 || visible)
         graph.Text("SALIR", option3X, menuStartY + spacing * 2);
    }
    
    // Dibujar indicador de selección a la izquierda
    if(visible)
    {
        int indicatorY = menuStartY + (spacing * selectedOption) + 5;
        int indicatorX = 120;
        graph.Text(">", indicatorX, indicatorY);
    }
}

int PMENU::DrawAll()
{
    PAPP::DrawScrollingBackground();
    DrawTitle();
    DrawMenu();
    
    graph.Flip();
    
    return 1;
}

void * PMENU::MoveAll()
{
    static bool upPressed = false;
    static bool downPressed = false;
    static bool enterPressed = false;

    if(cont>0) cont--;
    else cont = 30;

    // Actualizar el fondo scrolling compartido
    PAPP::UpdateScrollingBackground();

    // Mover el título hacia abajo más rápido y detenerse más arriba
    if(y<50) y+=10; // Posición final en y=50, más rápido
    else
        if(!cont) visible = !visible;

    // Navegación del menú
    if(y >= 50) // Solo permitir navegación cuando el título está en posición
    {
        if(input.Key(SDL_SCANCODE_UP) || input.Key(gameinf.keys[PLAYER1].left))
        {
            if(!upPressed)
            {
                selectedOption--;
                if(selectedOption < 0)
                    selectedOption = 2;
                upPressed = true;
            }
        }
        else
            upPressed = false;
            
        if(input.Key(SDL_SCANCODE_DOWN) || input.Key(gameinf.keys[PLAYER1].right))
        {
            if(!downPressed)
            {
                selectedOption++;
                if(selectedOption > 2)
                    selectedOption = 0;
                downPressed = true;
            }
        }
        else
            downPressed = false;
        
        // Seleccionar opción
        if(input.Key(SDL_SCANCODE_RETURN) || input.Key(gameinf.keys[PLAYER1].shoot))
        {
            if(!enterPressed)
            {
                switch(selectedOption)
                {
                    case 0: // JUGAR
                        return new PSELECT();
                    
                    case 1: // CONFIGURACIÓN
                        return new PCONFIGSCREEN();
                    
                    case 2: // SALIR
                        quit = 1;
                        return NULL;
                }
                enterPressed = true;
            }
        }
        else
            enterPressed = false;
    }
    
    return NULL;
}