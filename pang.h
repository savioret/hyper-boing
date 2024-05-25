#ifndef PANG_H_
#define PANG_H_

#include <windows.h>
#include <process.h>
#include <ddraw.h>
#include "app.h"
#include "resource.h"
#include "graph.h"
#include "bmfont.h"
#include "scene.h"
#include "menu.h"
#include "select.h"
#include "stageclear.h"
#include "mlist.h"
#include "pangmap.h"
#include "ball.h"
#include "player.h"
#include "item.h"
#include "shoot.h"
#include "floor.h"
#include "minput.h"

#define OBJ_NULL 0
#define OBJ_BALL 1
#define OBJ_SHOOT 2
#define OBJ_FLOOR 3
#define OBJ_ITEM 4
#define OBJ_PLAYER 5

#define ANIM_LEFT 0
#define ANIM_RIGHT 5
#define ANIM_SHOOT 10
#define ANIM_WIN 12
#define ANIM_DEAD 13

#define MAX_Y 415
#define MIN_Y 16
#define MAX_X 623
#define MIN_X 16

#define PLAYER1 0
#define PLAYER2 1

#define CHANGE_MAIN 1
#define CHANGE_SCENE 2

#define FONT_BIG 0
#define FONT_SMALL 1
#define FONT_HUGE 2

#define SIDE_TOP 1
#define SIDE_BOTTOM 2
#define SIDE_LEFT 3
#define SIDE_RIGHT 4


#define GLOBAL_GAMESPEED 60

typedef struct PGAMEBITMAPS
{
	SPRITE player[2][21];	
};



/********************************************************
  clase KEYS

  Esta clase contiene informacion acerda de las teclas del
  juego. Cada jugador tendrá una instancia de esta clase asociada.
********************************************************/
class KEYS
{
public:
	SDL_Scancode left;
	SDL_Scancode right;
	SDL_Scancode shoot;

	void SetLeft( SDL_Scancode _left) { this->left = _left; }
	void SetRight( SDL_Scancode _right) { this->right = _right; }
	void SetShoot( SDL_Scancode _shoot) { this->shoot = _shoot; }
	void Set( SDL_Scancode lf, SDL_Scancode rg, SDL_Scancode sh)
	{
		this->left = lf;
		this->right = rg;
		this->shoot = sh;
	}
};

/********************************************************
  clase PGAMEINFO

  Aqui es donde se almacena la informacion  comun del juego
  independientemente de la pantalla en la que nos encontremos.
********************************************************/
class PGAMEINFO
{
public:
	int numplayers;
	int numstages;
	PLAYER *player[2];
	PAPP *scene;
	KEYS keys[2];

	PGAMEBITMAPS bmp;

	int currentstage;
	PSTAGE stage[6];
	BOOL menu;
	
	PGAMEINFO();

	void Init();
	void InitStages();
	void SetCurrent(PAPP *app);
	void Release();
};

class PSCENE;

char OpenMusic( char * );
char PlayMusic( void );
char StopMusic( void );
char ContinueMusic( void );
char CloseMusic( void );
int Window_Create(HINSTANCE hInstance, int nCmdShow);
void Window_GetClientSize (HWND hwnd, int *sx, int *sy);
void Window_GetSize (HWND hwnd, int *sx, int *sy);

extern GRAPH graph; // Objeto para el modo grafico
extern MINPUT input; //Objeto para DirectInput, Funciones para teclado
extern PGAMEINFO gameinf;
extern KEYS keys[2];
extern int globalmode;
extern BOOL goback;

#endif