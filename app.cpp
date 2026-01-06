#include <windows.h>
#include "pang.h"

// Inicializar variables estáticas
SPRITE* PAPP::sharedBackground = nullptr;
int PAPP::scrollX = 0;
int PAPP::scrollY = 0;
bool PAPP::backgroundInitialized = false;

PAPP::PAPP()
{		
}

int PAPP::Init()
{
    // Inicialización común para todas las pantallas
    active = TRUE;
    pause = FALSE;
    SetGameSpeed(60);
    diftime1 = 0;
    diftime2 = gamespeed;
    time1 = GetTickCount() + gamespeed;
    time2 = GetTickCount();
    fps = 0;
    fpsv = 0;
    
    return 1;
}

void PAPP::InitSharedBackground()
{
    if (!backgroundInitialized)
    {
        sharedBackground = new SPRITE();
        sharedBackground->Init(&graph, "graph\\titleback.png", 0, 0);
        graph.SetColorKey(sharedBackground->bmp, RGB(255,0,0));
        scrollX = 0;
        scrollY = sharedBackground->sy;
        backgroundInitialized = true;
    }
}

void PAPP::UpdateScrollingBackground()
{
    if (!backgroundInitialized || !sharedBackground) return;
    
    if(scrollX < sharedBackground->sx) scrollX++;
    else scrollX = 0;

    if(scrollY > 0) scrollY--;
    else scrollY = sharedBackground->sy;
}

void PAPP::DrawScrollingBackground()
{
    if (!backgroundInitialized || !sharedBackground) return;
    
    int i, j;
    RECT rc, rcbx, rcby, rcq;
    
    rc.left = scrollX;
    rc.right = sharedBackground->sx;
    rc.top = 0;
    rc.bottom = scrollY;
    
    rcbx.left = 0;
    rcbx.right = scrollX;
    rcbx.top = 0;
    rcbx.bottom = scrollY;

    rcby.left = scrollX;
    rcby.right = sharedBackground->sx;
    rcby.top = scrollY;
    rcby.bottom = sharedBackground->sy;

    rcq.left = 0;
    rcq.right = scrollX;
    rcq.top = scrollY;
    rcq.bottom = sharedBackground->sy;
    
    for(i=0; i<4; i++)
        for(j=0; j<5; j++)
        {
            graph.Draw(sharedBackground->bmp, &rc, sharedBackground->sx*i, (sharedBackground->sy*j)+sharedBackground->sy-scrollY);
            graph.Draw(sharedBackground->bmp, &rcbx, (sharedBackground->sx*i)+rc.right-rc.left, (sharedBackground->sy*j)+sharedBackground->sy - scrollY);
            graph.Draw(sharedBackground->bmp, &rcby, sharedBackground->sx*i, sharedBackground->sy*j);
            graph.Draw(sharedBackground->bmp, &rcq, (sharedBackground->sx*i)+sharedBackground->sx-scrollX, sharedBackground->sy*j);
        }
}

void PAPP::ReleaseSharedBackground()
{
    if (backgroundInitialized && sharedBackground)
    {
        sharedBackground->Release();
        delete sharedBackground;
        sharedBackground = nullptr;
        backgroundInitialized = false;
    }
}

/*******************************************************
  Esta funcion es primordial en juego.
  Aqui se administra el numero de veces que se va a pintar, 
  como maximo el establecido, y se calcularan tantas 
  veces como se indique (variable gamespeed).

  Por ejemplo, si queremos pintar 60 fotogramas por segundo
  mediante un contador, controlaremos que  calculamos 60, 
  y si nos da tiempo pintaremos 60, en caso de que tengamos
  un equipo mas lento, tan solo pintaremos los que nuestro
  ordenador pueda, pero la velocidad "virtual" del juego
  seguira siendo de 60 fps.
*******************************************************/
void * PAPP::DoTick()
{
    static short framestatus = 0;	
    static short cont =0;
    static long tick, lasttick;

    if (goback)
    {
        goback = FALSE;
        gameinf.menu = TRUE;
        return (PAPP*) new PMENU;
    }

    if(framestatus==0)
    {
        time1=GetTickCount();
        diftime2=time1-time2;
        if(diftime2< gamespeed) return NULL;
        time2=time1;
        diftime1+=diftime2;
        framestatus=1;
        return FALSE;
    }

    if (framestatus==1)
    {
        if(diftime1 < gamespeed)
        {
            framestatus=2;
            return NULL;
        }		
        void * newscreen = (PAPP*) MoveAll();
        diftime1-=gamespeed;
        return (PAPP*) newscreen;
    }
    
    if(framestatus==2)
    {
        DrawAll();
        framestatus=0;
        tick=GetTickCount();
        if(tick-lasttick>1000)
        {
          fps = cont;
            cont = 0;
            lasttick = tick;
        }
        else
            cont++; //Frames por segundo
    }

    return NULL;
}

/*******************************************************
    Si hacemos una pausa tenemos que actualizar estos datos
    de manera que la funcion DoTick actue correctamente una vez
    reanudemos el juego.
*******************************************************/

void PAPP::DoPause()
{	
    diftime1=0; 
    diftime2=gamespeed;
    time1 = GetTickCount()+gamespeed;
    time2 = GetTickCount();

    return;
}

/*******************************************************
    Ajustamos la velocidad de pintado en fotogramas por segundo
    y lo transformamos a los milisegundos que equivalen
    por cada fotograma, que al fin y al cabo es el dato que
    nos va a hacer falta.
*********************************************************/
void PAPP::SetGameSpeed(int speed)
{
    gamespeed = 1000/speed;
}

/*******************************************************
    Indicamos si nuestra aplicacion esta o no activa
*********************************************************/
void PAPP::SetActive(BOOL b)
{
    active = b;
}

void PAPP::SetPause(BOOL b)
{
    pause = b;
}

BOOL PAPP::IsActive()
{
    return active;
}

BOOL PAPP::IsPaused()
{
    return pause;
}

