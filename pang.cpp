#include "pang.h"
#include "app.h"

#include <stdio.h>
#include <iostream>


int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int );
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );


//--- Declaración de variables del programa ------------------------------
int quit=0, minimised=0;
BOOL goback = FALSE;
int globalmode;
char WindowName[]  = "Chuper Pang Revolution";
char WindowTitle[] = "¡Chuper Pang!";
HDC hdc;
HINSTANCE hInst;
HWND hWndMain;
FARPROC lpfnDIALOGSMsgProc;
HWND hConfigDlg;


GRAPH graph; // Objeto para el modo grafico
MINPUT input; //Objeto para DirectInput, Funciones para teclado
PGAMEINFO gameinf;
PAPP *screen, *nextscreen;

//=== Función principal WinMain() ========================================
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow )
{
    MSG msg;                                // estructura de mensaje
    
    AllocConsole ();
    FILE* fDummy;
    freopen_s ( &fDummy, "CONOUT$", "w", stdout );
    freopen_s ( &fDummy, "CONOUT$", "w", stderr );
    freopen_s ( &fDummy, "CONIN$", "r", stdin );

    printf ( "Initializing....\n" );
    screen = new PMENU;


    hInst = hInstance;
    if (!Window_Create(hInstance, nCmdShow))
        return FALSE;
    
    gameinf.Init();
    screen->Init();  /*###### INICIO DEL JUEGO ######*/

    // Main message loop
    // Main message loop
    SDL_Event e;
    while ( !quit )
    {
        while ( SDL_PollEvent( &e ) )
        {
            if ( e.type == SDL_QUIT ) { quit = 1; }
            if ( e.type == SDL_KEYDOWN )
            {
                 // Handle global keys if needed or pass to input system
                 if ( e.key.keysym.sym == SDLK_ESCAPE )
                 {
                     if(!gameinf.menu)
                        goback = TRUE;
                     else
                     {
                         // Basic exit confirmation could be here, or just let menu handle it
                         // For now, let menu handle 'quit' flag via input.
                     }
                 }
                 if(e.key.keysym.sym == SDLK_p)
                 {
                     screen->SetPause(!screen->IsPaused());
                 }
                 if(e.key.keysym.sym == SDLK_c)
                 {
                     // Config screen
                     screen = new PCONFIGSCREEN();
                     screen->Init();
                 }
            }
        }

        if ( quit ) break;

        if ( screen->IsPaused () )
            screen->DoPause ();
        else
        {
            nextscreen = (PAPP*)screen->DoTick ();
            if ( nextscreen != NULL )
            {
                screen->Release ();
                screen = nextscreen;
                gameinf.SetCurrent ( screen );
                screen->Init ();
            }
        }
        
        // Add a small delay to prevent 100% CPU usage if DoTick doesn't cap speed properly?
        // DoTick seems to handle timing.
    }

    graph.Release();
    CloseMusic();

    // Free the console
    FreeConsole ();

    // devolvemos el valor recibido por PostQuitMessage().
    return 0;
}


//=== Función del procedimiento de ventana WndProc() =====================
// WndProc is no longer used in SDL mode
LRESULT CALLBACK WndProc( HWND hwnd, UINT message, 
                          WPARAM wParam, LPARAM lParam )
{
    return 0;
}

int Window_Create(HINSTANCE hInstance, int nCmdShow)
{
    // Skip Win32 Window Creation for SDL2
    /*
    int sx, sy, csx, csy;
    WNDCLASS wc;
    ...
    */
    
    hWndMain = NULL; // No Win32 window

    // Establecer modo ventana por defecto (eliminamos el diálogo inicial)
    globalmode = RENDERMODE_NORMAL;

    if(!input.Init(hInst, hWndMain))
    {
        printf("Error init Input\n");
        return FALSE;
    }
    
    // Initialize Graphics with SDL
    if (!graph.Init("Chuper Pang Revolution", globalmode))
    {
        printf("Error Init SDL\n");
        return FALSE;
    }
    
    return TRUE;
}


void Window_GetClientSize (HWND hwnd, int *sx, int *sy)
{
  RECT rect;
  GetClientRect (hwnd, &rect);
  *sx = rect.right-rect.left+1;
  *sy = rect.bottom-rect.top+1;
}

void Window_GetSize (HWND hwnd, int *sx, int *sy)
{
  RECT rect;
  GetWindowRect (hwnd, &rect);
  *sx = rect.right-rect.left+1;
  *sy = rect.bottom-rect.top+1;
}


/****************** FUNCIONES DE GAMEINFO ********************/

PGAMEINFO::PGAMEINFO()
{
    currentstage = 1;
    numplayers = 1;
    player[PLAYER1] = player[PLAYER2] = NULL;
    
}

void PGAMEINFO::Init()
{
    int  i;
    menu = TRUE;

    bmp.player[PLAYER1][0].Init(&graph,"graph\\p1k1l.png", 0, 3);
    bmp.player[PLAYER1][1].Init(&graph,"graph\\p1k2l.png", 4, 3);
    bmp.player[PLAYER1][2].Init(&graph,"graph\\p1k3l.png", 6, 3);
    bmp.player[PLAYER1][3].Init(&graph,"graph\\p1k4l.png", 4, 3);
    bmp.player[PLAYER1][4].Init(&graph,"graph\\p1k5l.png", 4, 3);
    
    bmp.player[PLAYER1][5].Init(&graph,"graph\\p1k1r.png", 0, 3);
    bmp.player[PLAYER1][6].Init(&graph,"graph\\p1k2r.png", 0, 3);
    bmp.player[PLAYER1][7].Init(&graph,"graph\\p1k3r.png", 0, 3);
    bmp.player[PLAYER1][8].Init(&graph,"graph\\p1k4r.png", 0, 3);
    bmp.player[PLAYER1][9].Init(&graph,"graph\\p1k5r.png", 0, 3);

    bmp.player[PLAYER1][10].Init(&graph,"graph\\p1shoot1.png", 13, 0);
    bmp.player[PLAYER1][11].Init(&graph,"graph\\p1shoot2.png", 13, 3);
    bmp.player[PLAYER1][12].Init(&graph,"graph\\p1win.png", 13, 4);
    bmp.player[PLAYER1][13].Init(&graph,"graph\\p1dead.png");
    bmp.player[PLAYER1][14].Init(&graph,"graph\\p1dead2.png");
    bmp.player[PLAYER1][15].Init(&graph,"graph\\p1dead3.png");
    bmp.player[PLAYER1][16].Init(&graph,"graph\\p1dead4.png");
    bmp.player[PLAYER1][17].Init(&graph,"graph\\p1dead5.png");
    bmp.player[PLAYER1][18].Init(&graph,"graph\\p1dead6.png");
    bmp.player[PLAYER1][19].Init(&graph,"graph\\p1dead7.png");
    bmp.player[PLAYER1][20].Init(&graph,"graph\\p1dead8.png");

    for(i=0; i<21;i++)
        graph.SetColorKey(bmp.player[PLAYER1][i].bmp, RGB(0,255,0));

    bmp.player[PLAYER2][0].Init(&graph,"graph\\p2k1l.png", 0, 3);
    bmp.player[PLAYER2][1].Init(&graph,"graph\\p2k2l.png", 4, 3);
    bmp.player[PLAYER2][2].Init(&graph,"graph\\p2k3l.png", 6, 3);
    bmp.player[PLAYER2][3].Init(&graph,"graph\\p2k4l.png", 4, 3);
    bmp.player[PLAYER2][4].Init(&graph,"graph\\p2k5l.png", 4, 3);
    
    bmp.player[PLAYER2][5].Init(&graph,"graph\\p2k1r.png", 0, 3);
    bmp.player[PLAYER2][6].Init(&graph,"graph\\p2k2r.png", 0, 3);
    bmp.player[PLAYER2][7].Init(&graph,"graph\\p2k3r.png", 0, 3);
    bmp.player[PLAYER2][8].Init(&graph,"graph\\p2k4r.png", 0, 3);
    bmp.player[PLAYER2][9].Init(&graph,"graph\\p2k5r.png", 0, 3);

    bmp.player[PLAYER2][10].Init(&graph,"graph\\p2shoot1.png", 13, 0);
    bmp.player[PLAYER2][11].Init(&graph,"graph\\p2shoot2.png", 13, 3);
    bmp.player[PLAYER2][12].Init(&graph,"graph\\p2win.png", 13, 4);
    bmp.player[PLAYER2][13].Init(&graph,"graph\\p2dead.png");
    bmp.player[PLAYER2][14].Init(&graph,"graph\\p2dead2.png");
    bmp.player[PLAYER2][15].Init(&graph,"graph\\p2dead3.png");
    bmp.player[PLAYER2][16].Init(&graph,"graph\\p2dead4.png");
    bmp.player[PLAYER2][17].Init(&graph,"graph\\p2dead5.png");
    bmp.player[PLAYER2][18].Init(&graph,"graph\\p2dead6.png");
    bmp.player[PLAYER2][19].Init(&graph,"graph\\p2dead7.png");
    bmp.player[PLAYER2][20].Init(&graph,"graph\\p2dead8.png");

    for(i=0; i<21;i++)
        graph.SetColorKey(bmp.player[PLAYER2][i].bmp, RGB(255,0,0));
    //-------------------------------------------------------------//

    keys[PLAYER1].Set ( SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_SPACE );
    keys[PLAYER2].Set ( SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_LCTRL );
}

void PGAMEINFO::InitStages()
{
    /*************** INICIO DE LAS PANTALLAS ****************/
    /***** EL CODIGO SIGUIENTE DEBERIA HACERSE LEYENDO *****/
    /***** DESDE UN FICHERO, PERO POR FALTA DE TIEMPO  *****/
    /***** LO VAMOS A HACER A MANO.					   *****/
    int i;
    int x,y;
    PEXTRA extra;

    numstages = 6;

    for(i=0;i<numstages;i++)
        stage[i].Reset();
    


/*===============================================*/
    i=0;
    ZeroMemory(&extra, sizeof(PEXTRA));
    stage[i].xpos[PLAYER1] =250;
    stage[i].xpos[PLAYER2] = 350;
    stage[i].SetBack("fondo1.png");
    stage[i].SetMusic("stage1.mid");
    stage[i].timelimit = 100;
    stage[i].id = i+1;  //pantalla 1
    stage[i].Add(OBJ_FLOOR, 550, 50, 0);		
    stage[i].Add(OBJ_FLOOR, 250, 250, 0);
    extra.ex1 = 1;
    stage[i].AddX(OBJ_FLOOR, 350, 150, 0, extra);
    stage[i].AddX(OBJ_FLOOR, 550, 150, 0, extra);
    extra.ex1 = 0;
    extra.ex2 = 0;
    extra.ex3 = 1;
    extra.ex4 = 1;
    stage[i].Add(OBJ_BALL, 1);  // añade a la secuencia en tiempo =1, tamaño=3
    for ( x = 10; x < 650; x += 64 )
        if ( x < 250 || x> 350 )
        {
            stage[i].AddX ( OBJ_BALL, x, 395, 2, extra );  // añade a la secuencia en tiempo =1, tamaño=3		
            stage[i].AddX ( OBJ_BALL, x, 180, 2, extra );
        }

    extra.ex1 = 0;
    extra.ex2 = 0;
    extra.ex3 = 0;
    extra.ex4 = 0;
    stage[i].Add(OBJ_BALL, 20); // en tiempo=20



    /*============================================*/
    i=1;
    stage[i].xpos[PLAYER1] =  stage[i].xpos[PLAYER2] = 270;
    stage[i].SetBack("fondo2.png");
    stage[i].SetMusic("stage4.mid");
    stage[i].timelimit = 100;
    stage[i].id = i+1;  //pantalla 1	
    extra.ex1 = 3;  // tamaño
    extra.ex2 = 200; // top
    extra.ex3 = 0; // dirx
    extra.ex4 = 1; // diry
    for(y=0;y<2; y++)
    {
        if(y==0)extra.ex3 = -1; // dirx
        else extra.ex3 = 1; // dirx
        for(x=0;x<10;x++)		
            stage[i].AddX(OBJ_BALL, +128 + 300*y+x*16, 100, 1, extra);  // añade a la secuencia en tiempo =1, tamaño=3					
    }

    /*============================================*/
    i=2;
    stage[i].xpos[PLAYER1] =200;
    stage[i].xpos[PLAYER2] = 350;
    stage[i].SetBack("fondo1.png");
    stage[i].SetMusic("stage5.mid");
    stage[i].timelimit = 100;
    stage[i].id = i+1;
    stage[i].Add(OBJ_FLOOR, 250, 70, 0);			
    stage[i].Add(OBJ_BALL, 1);  
    stage[i].Add(OBJ_BALL, 1);  
    extra.ex1 = 2;  // tamaño
    extra.ex2 = 0; // top asignado automaticamente
    extra.ex3 = 1; // dirx
    extra.ex4 = 1; // diry
    stage[i].AddX(OBJ_BALL, rand()%600+32, 400, 1, extra);  // añade a la secuencia en tiempo =1, tamaño=3, altura=400
    extra.ex3 = -1; // dirx	
    stage[i].AddX(OBJ_BALL, rand()%600+32, 400, 1, extra);  // añade a la secuencia en tiempo =1, tamaño=3, altura=400

    /*===============================================*/
    i=3;
    stage[i].SetBack("fondo3.png");
    stage[i].SetMusic("stage3.mid");
    stage[i].timelimit = 100;
    stage[i].id = i+1;  //pantalla 1
    extra.ex1=0;
    stage[i].Add(OBJ_FLOOR, 250, 70, 0);	
    extra.ex1 = 3;
    stage[i].AddX(OBJ_BALL, 1, extra); 
    stage[i].Add(OBJ_BALL, 1); 
    stage[i].Add(OBJ_BALL, 20);


        /*===============================================*/
    i=4;
    ZeroMemory(&extra, sizeof(PEXTRA));
    stage[i].xpos[PLAYER1] =250;
    stage[i].xpos[PLAYER2] = 350;
    stage[i].SetBack("fondo5.png");
    stage[i].SetMusic("stage4.mid");
    stage[i].timelimit = 100;
    stage[i].id = i+1;  //pantalla 1
    stage[i].Add(OBJ_FLOOR, 16, 100, 0);
    extra.ex1 = 1;

    stage[i].Add(OBJ_FLOOR, 80, 164, 0);
    extra.ex1 = 1;
    stage[i].AddX(OBJ_FLOOR, 144, 164, 0, extra);
    stage[i].Add(OBJ_FLOOR, 144, 228, 0);
    extra.ex1 = 1;
    stage[i].AddX(OBJ_FLOOR, 208, 228, 0, extra);
    stage[i].Add(OBJ_FLOOR, 208, 292, 0);

    stage[i].Add(OBJ_FLOOR, RES_X-80, 100, 0);
    extra.ex1 = 1;

    stage[i].Add(OBJ_FLOOR, RES_X-128, 164, 0);
    extra.ex1 = 1;
    stage[i].AddX(OBJ_FLOOR, RES_X-144, 164, 0, extra);
    stage[i].Add(OBJ_FLOOR, RES_X-192, 228, 0);
    extra.ex1 = 1;
    stage[i].AddX(OBJ_FLOOR, RES_X-208, 228, 0, extra);
    stage[i].Add(OBJ_FLOOR, RES_X-256, 292, 0);

    extra.ex1 = 3;  // tamaño	
    extra.ex3 = 1; // dirx
    extra.ex4 = 1; // diry		
    for(x=0;x<15;x++)
    {
        extra.ex2 = rand()%150 + 150;// altura maxima de salto
        extra.ex3 = 1; // dirx
        stage[i].AddX(OBJ_BALL, 17, 50, 5*x, extra);  		
        extra.ex3 = -1; // dirx
        stage[i].AddX(OBJ_BALL, MAX_X-30, 50, 5*x, extra);  
    }

    /*===============================================*/
    i=5;
    
    stage[i].xpos[PLAYER1] =250;
    stage[i].xpos[PLAYER2] = 350;
    stage[i].SetBack("fondo4.png");
    stage[i].SetMusic("stage2.mid");
    stage[i].timelimit = 100;
    stage[i].id = i+1;  //pantalla 1
    extra.ex1 = 1;
    for(x=56;x<600;x+=64)
        for(y=22;y<288;y+=64)
            stage[i].AddX(OBJ_FLOOR, x, y, 0, extra);
    extra.ex1 = 1;  // tamaño
    extra.ex2 = 395;
    extra.ex3 = 0;
    extra.ex4 = 1;
    for(x=10;x<650;x+=64)	
        if(x<250 || x> 350)
        {
            stage[i].AddX(OBJ_BALL, x, 395, 1, extra);  // añade a la secuencia en tiempo =1, tamaño=3		
            stage[i].AddX(OBJ_BALL, x, 150, 1, extra);
        }	
    
}
void PGAMEINFO::SetCurrent(PAPP *app)
{
    scene = app;
}


void PGAMEINFO::Release()
{
    int i;

    for(i=0;i<14;i++)
    {
        bmp.player[PLAYER1][i].Release();
        bmp.player[PLAYER2][i].Release();
    }
}
//=== Funciones MCI de sonido ======================================

char OpenMusic( char *filename )
{
  char buffer[256];
  wsprintf(buffer, "open %s type sequencer alias MUSIC", filename );
  return( mciSendString( buffer, 0, 0, 0 ) == 0 );
}

char PlayMusic( void )
{
  return( mciSendString("play MUSIC from 0 notify", 0, 0, 0 ) == 0 );
}

char StopMusic( void )
{
  return( mciSendString("stop MUSIC", 0, 0, 0 ) == 0 );
}

char ContinueMusic( void )
{
  // continuar desde el último Stop que se hizo:
  return( mciSendString("play MUSIC notify", 0, 0, 0 ) == 0 );
}
 
char CloseMusic( void )
{
  // para permitir la reprod. de otro MIDI.
  // recomendable también antes de PlaySound();
  return( mciSendString("close all", 0, 0, 0 ) == 0 );
}
//=== Fin del archivo ====================================================//=== Fin del archivo ====================================================//=== Fin del archivo ====================================================//=== Fin del archivo ====================================================//=== Fin del archivo ====================================================//=== Fin del archivo ====================================================//=== Fin del archivo ====================================================//=== Fin del archivo ====================================================