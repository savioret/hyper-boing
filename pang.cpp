#include "pang.h"
#include "app.h"

int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR, int );
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );
BOOL FAR PASCAL DIALOG1MsgProc(HWND hWndDlg, WORD Message, WORD wParam, LONG lParam);
BOOL FAR PASCAL ConfigDlgMsgProc(HWND hWndDlg, WORD Message, WORD wParam, LONG lParam);
BOOL FAR PASCAL InitDlgMsgProc(HWND hWndDlg, WORD Message, WORD wParam, LONG lParam);
int ShowConfigDlg( HINSTANCE hInst, HWND hWnd);
int ShowInitDlg( HINSTANCE hInst, HWND hWnd);


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
	screen = new PMENU;

	hInst = hInstance;
	if (!Window_Create(hInstance, nCmdShow)) return FALSE;
	
	gameinf.Init();
	screen->Init();  /*###### INICIO DEL JUEGO ######*/
 
  // etc... 
 while (!quit)
 {
  while (PeekMessage(&msg,NULL,0,0,PM_REMOVE) )
  {
    if (msg.message == WM_QUIT) { quit=1; break; }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  if (quit) break;

  if (minimised)
  	WaitMessage(); 
  
   else
   {
	   if(screen->IsPaused())	
		screen->DoPause();
	   else
	   {
		  nextscreen = (PAPP*) screen->DoTick();
		  if( nextscreen != NULL)
		  {		
			screen->Release();
			screen = nextscreen;
			gameinf.SetCurrent(screen);
			screen->Init();
		  }
	   }
   }

 }
	graph.Release();
	CloseMusic();

	// devolvemos el valor recibido por PostQuitMessage().
	return( msg.wParam );
}


//=== Función del procedimiento de ventana WndProc() =====================
LRESULT CALLBACK WndProc( HWND hwnd, UINT message, 
						  WPARAM wParam, LPARAM lParam )
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;

	switch( message )
	{
    case WM_COMMAND:                                        
	       switch (wParam)
           {
            case IDM_CONS_NEW:
//				scene.map.New();
                 break;

			case IDM_CONFIG:
				screen->SetPause(TRUE);
				ShowConfigDlg(hInst,hWndMain);
			break;

			case IDM_SALIR:
				screen->SetPause(TRUE);
				if(MessageBox(hWndMain, "Estas seguro de querer salir?", "SALIR", MB_YESNO | MB_ICONQUESTION) == IDYES)
					PostQuitMessage(0);
				screen->SetPause(FALSE);
			break;

				 
            default:
                return DefWindowProc(hwnd, message, wParam, lParam);
           }   		 
         break;

		// mensaje producido en la creación de la ventana
		case WM_CREATE:				
				//scene.hFondo = LoadBitmap(hInst, "IDB_FONDO");				
				break;
		case WM_ACTIVATE:
			screen->SetActive((BOOL) wParam);
			if(screen->IsActive() )
			{
				// necesitamos reactivar el teclado
				input.ReacquireInput();
			}
            
		break;

		case WM_KEYDOWN:
    		switch(wParam)
    		{
    			case VK_ESCAPE:
					if(!gameinf.menu)
	    				goback = TRUE;
					else
					{
						if(MessageBox(hWndMain, "Estas seguro de querer salir?", "SALIR", MB_YESNO | MB_ICONQUESTION) == IDYES)
						PostQuitMessage(0);
					}

				break;

				case 'p':
    			case 'P':
	    			screen->SetPause(!screen->IsPaused());
				break;
			}
		break;

		break;
	
		// mensaje producido al cerrar la ventana
		case WM_DESTROY:
				screen->Release();
				PostQuitMessage( 0 );
				break;

		// resto de mensajes, dar una respuesta estándar.
		default:
				return( DefWindowProc( hwnd, message, wParam, lParam ) );
	}

	return(0);
}

int Window_Create(HINSTANCE hInstance, int nCmdShow)
{
	int sx, sy, csx, csy;

  WNDCLASS wc;
  
  wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon( hInstance, "IDI_ICONPANG" );
  wc.hCursor = NULL;
  wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = "MiClase";
  if (!RegisterClass(&wc)) return FALSE;
  
  hWndMain = CreateWindow ("MiClase", "HOLA!!", 
	  WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_POPUP | WS_MINIMIZEBOX, 
	  0,0,640,480, NULL, 
	  LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1)), 
	  hInstance, NULL);

  if (hWndMain==NULL)
  {
	UnregisterClass ("MiClase", hInstance);
	return FALSE;
  }


	//-----------------------------------------------------------------//

	// Hacemos visible la ventana y la actualizamos:
	MoveWindow (hWndMain,0,0,RES_X,RES_Y, TRUE);
    Window_GetSize (hWndMain, &sx, &sy);
    Window_GetClientSize (hWndMain, &csx, &csy);

    MoveWindow (hWndMain,(GetSystemMetrics(SM_CXSCREEN)-RES_X)/2,(GetSystemMetrics(SM_CYSCREEN)-(RES_Y+(sy-csy)))/2,RES_X+(sx-csx), RES_Y+(sy-csy), TRUE);

	ShowWindow( hWndMain, nCmdShow );
	UpdateWindow( hWndMain );
	SetFocus(hWndMain);


	if(!ShowInitDlg(hInst, hWndMain))
		PostQuitMessage(0);

	if(!input.Init(hInst, hWndMain))
	{
		MessageBox(hWndMain, "Error iniciando Direct Input \n ¿tienes instaladas las DirectX version 7?",0,0);
		return FALSE;
	}
	else
	if (!graph.Init(hWndMain, globalmode))
	{
		MessageBox(hWndMain, "Error iniciando Direct Draw \n ¿tienes instaladas las DirectX version 7?",0,0);;
		return FALSE;
	}
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


int ShowConfigDlg( HINSTANCE hInst, HWND hWnd)
{
	int        nRc;
    
    nRc = DialogBox(hInst, "IDD_KEYBCONFIG", hWnd, (DLGPROC) ConfigDlgMsgProc);

	return nRc;
}

int ShowInitDlg( HINSTANCE hInst, HWND hWnd)
{
	int        nRc;
    
    nRc = DialogBox(hInst, "IDD_INIT", hWnd, (DLGPROC) InitDlgMsgProc);

	return nRc;
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

	bmp.player[PLAYER1][0].Init(&graph,"graph\\p1k1l.bmp", 0, 3);
	bmp.player[PLAYER1][1].Init(&graph,"graph\\p1k2l.bmp", 4, 3);
	bmp.player[PLAYER1][2].Init(&graph,"graph\\p1k3l.bmp", 6, 3);
	bmp.player[PLAYER1][3].Init(&graph,"graph\\p1k4l.bmp", 4, 3);
	bmp.player[PLAYER1][4].Init(&graph,"graph\\p1k5l.bmp", 4, 3);
	
	bmp.player[PLAYER1][5].Init(&graph,"graph\\p1k1r.bmp", 0, 3);
	bmp.player[PLAYER1][6].Init(&graph,"graph\\p1k2r.bmp", 0, 3);
	bmp.player[PLAYER1][7].Init(&graph,"graph\\p1k3r.bmp", 0, 3);
	bmp.player[PLAYER1][8].Init(&graph,"graph\\p1k4r.bmp", 0, 3);
	bmp.player[PLAYER1][9].Init(&graph,"graph\\p1k5r.bmp", 0, 3);

	bmp.player[PLAYER1][10].Init(&graph,"graph\\p1shoot1.bmp", 13, 0);
	bmp.player[PLAYER1][11].Init(&graph,"graph\\p1shoot2.bmp", 13, 3);
	bmp.player[PLAYER1][12].Init(&graph,"graph\\p1win.bmp", 13, 4);
	bmp.player[PLAYER1][13].Init(&graph,"graph\\p1dead.bmp");
	bmp.player[PLAYER1][14].Init(&graph,"graph\\p1dead2.bmp");
	bmp.player[PLAYER1][15].Init(&graph,"graph\\p1dead3.bmp");
	bmp.player[PLAYER1][16].Init(&graph,"graph\\p1dead4.bmp");
	bmp.player[PLAYER1][17].Init(&graph,"graph\\p1dead5.bmp");
	bmp.player[PLAYER1][18].Init(&graph,"graph\\p1dead6.bmp");
	bmp.player[PLAYER1][19].Init(&graph,"graph\\p1dead7.bmp");
	bmp.player[PLAYER1][20].Init(&graph,"graph\\p1dead8.bmp");

	for(i=0; i<21;i++)
		graph.SetColorKey(bmp.player[PLAYER1][i].bmp, RGB(0,255,0));

	bmp.player[PLAYER2][0].Init(&graph,"graph\\p2k1l.bmp", 0, 3);
	bmp.player[PLAYER2][1].Init(&graph,"graph\\p2k2l.bmp", 4, 3);
	bmp.player[PLAYER2][2].Init(&graph,"graph\\p2k3l.bmp", 6, 3);
	bmp.player[PLAYER2][3].Init(&graph,"graph\\p2k4l.bmp", 4, 3);
	bmp.player[PLAYER2][4].Init(&graph,"graph\\p2k5l.bmp", 4, 3);
	
	bmp.player[PLAYER2][5].Init(&graph,"graph\\p2k1r.bmp", 0, 3);
	bmp.player[PLAYER2][6].Init(&graph,"graph\\p2k2r.bmp", 0, 3);
	bmp.player[PLAYER2][7].Init(&graph,"graph\\p2k3r.bmp", 0, 3);
	bmp.player[PLAYER2][8].Init(&graph,"graph\\p2k4r.bmp", 0, 3);
	bmp.player[PLAYER2][9].Init(&graph,"graph\\p2k5r.bmp", 0, 3);

	bmp.player[PLAYER2][10].Init(&graph,"graph\\p2shoot1.bmp", 13, 0);
	bmp.player[PLAYER2][11].Init(&graph,"graph\\p2shoot2.bmp", 13, 3);
	bmp.player[PLAYER2][12].Init(&graph,"graph\\p2win.bmp", 13, 4);
	bmp.player[PLAYER2][13].Init(&graph,"graph\\p2dead.bmp");
	bmp.player[PLAYER2][14].Init(&graph,"graph\\p2dead2.bmp");
	bmp.player[PLAYER2][15].Init(&graph,"graph\\p2dead3.bmp");
	bmp.player[PLAYER2][16].Init(&graph,"graph\\p2dead4.bmp");
	bmp.player[PLAYER2][17].Init(&graph,"graph\\p2dead5.bmp");
	bmp.player[PLAYER2][18].Init(&graph,"graph\\p2dead6.bmp");
	bmp.player[PLAYER2][19].Init(&graph,"graph\\p2dead7.bmp");
	bmp.player[PLAYER2][20].Init(&graph,"graph\\p2dead8.bmp");

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
	stage[i].SetBack("fondo1.bmp");
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
	stage[i].SetBack("fondo2.bmp");
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
	stage[i].SetBack("fondo1.bmp");
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
	stage[i].SetBack("fondo3.bmp");
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
	stage[i].SetBack("fondo5.bmp");
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
	stage[i].SetBack("fondo4.bmp");
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
		bmp.player[PLAYER1][i].bmp->Release();
		bmp.player[PLAYER2][i].bmp->Release();
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
//=== Fin del archivo ====================================================