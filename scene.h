#ifndef _SCENE_H_
#define _SCENE_H_

#include "bmfont.h"
#include "mlist.h"
#include "player.h"
#include "ball.h"
#include "shoot.h"
#include "floor.h"
#include "item.h"
#include "stage.h"

class PSTAGECLEAR;


/************************************************************
      clase FLOORCOLISION
  
    Es una clase auxiliar que necesito para tratar las
    colisiones de las BALLS con los FLOORS.
*************************************************************/
class FLOORCOLISION
{
public:
    FLOOR *fl;
    POINT col;

    FLOORCOLISION()
    { fl=NULL; col.x=col.y=0; }
};


typedef struct PSCNBITMAPS
{
    SPRITE back;
    SPRITE redball[4];
    SPRITE floor[2];
    SPRITE shoot[3];
    SPRITE mark[5];
    SPRITE fontnum[3];
    SPRITE miniplayer[2];
    SPRITE lives[2];
    SPRITE gameover;
    SPRITE continu;  // continue = palabra reservada :P
    SPRITE time;

}PSCENEBITMAPS;


/************************************************************
      clase PSTAGE
  
    Como se puede observar, puesto que es una clase que 
    hereda PAPP, se trata de un modulo del juego. Este es
    el modulo principal, donde se juega realmente.

   Contiene informacion con los bitmaps de la pantalla
   y varias listas enlazadas, con las pelotas en pantalla
   los items(opcion no implementada) los bloques y los disparos.

*************************************************************/

class PSCENE : public PAPP
{
public:

    PSCENEBITMAPS bmp;
    BMNUMFONT fontnum[3];

    MLIST lsballs;  // LISTA DE PELOTAS EN PANTALLA
    MLIST lsitems; // LISTA DE ITEMS
    MLIST lsfloor; // LISTA DE LAS OBSTRUCCIONES
    MLIST lsshoots; //disparos

    int levelclear;
    PSTAGECLEAR *pstageclear; //secuencia de nivel completado
    BOOL gameover;
    BOOL gameovercount; // cuenta atras para para continuar o no
    PSTAGE *stage; // informacion para el desarrollo del nivel
    
    int change;
    int dsecond; //contador de decimas de segundo;
    int time;  // cuenta atras
    int timeline; // linea de tiempo

    PSCENE(PSTAGE *stg, PSTAGECLEAR *pstgclr=NULL);
    
    void AddBall(int x=250, int y=-20, int size=0, int top=0, int dirx=1, int diry=1, int id=0);
    void AddItem(int x, int y, int id);
    void AddShoot(PLAYER *pl);
    void AddFloor(int x, int y, int id);
    void Shoot(PLAYER *pl);

    int DivideBall(BALL *b);
    void CheckColisions();
    void Decide(BALL *b, FLOORCOLISION *fc, int moved);

    int ObjectScore(int id);
    void Win();
    
    int Init();
    int InitBitmaps();
    void DrawBackground();
    int DrawAll();
    void Draw(BALL *b);
    void Draw(PLAYER *pl);
    void Draw(SHOOT *sht);
    void Draw(FLOOR *fl);
    void DrawScore();
    void DrawMark();

    void * MoveAll();
    void CheckSequence();

    int Release();
};


char OpenMusic( char * );
char PlayMusic( void );
char StopMusic( void );
char ContinueMusic( void );
char CloseMusic( void );

#endif
