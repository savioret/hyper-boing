#ifndef PAPP_H_
#define PAPP_H_

// Forward declaration
class SPRITE;

/********************************************************
 clase PAPP

  Esta clase se utiliza para crear diferentes instancias
  de la pantalla del juego en la que nos encontremos.
  Será la clase Padre de una serie de clases que forman
  parte del juego, a modo de "modulo" independiente.

  De ahí las funciones virtuales puras abajo declaradas:

  - Init();
		Cada modulo llevará asociado unos bitmaps y recursos
		necesarios, que se cargaran al crearse el objeto.
		De esta manera los recursos del sistema se administran
		mejor.
  - DrawAll();
		Dibuja todos los elementos que hay en pantalla.
  
  - MoveAll();
		Realiza todos los movimientos necesarios, lectura del
		teclado, y comprobaciones de colisiones si fuera
		necesario, etc...
  - ReleaseAll();
		Libera todos los recursos empleados en Init();
********************************************************/

class PAPP
{
public:
	
	int gamespeed; // fotogramas por segundo
	int fps, fpsv;
	BOOL active; // ventana activa ??
	BOOL pause;
	short int diftime1, diftime2;
	long time1, time2;

	// Background scrolling compartido
	static SPRITE* sharedBackground;
	static int scrollX, scrollY;
	static bool backgroundInitialized;

	void * DoTick();
	void DoPause();
	void SetGameSpeed(int speed);
	void SetPause(BOOL b);
	void SetActive(BOOL b);
	BOOL IsActive();
	BOOL IsPaused();

	PAPP();

	// Init es ahora virtual (no pura) para que las clases derivadas puedan llamarlo
	virtual int Init();
	virtual void * MoveAll()=0;
	virtual int DrawAll()=0;
	virtual int Release()=0;
	
	// Funciones compartidas
	static void InitSharedBackground();
	static void UpdateScrollingBackground();
	static void DrawScrollingBackground();
	static void ReleaseSharedBackground();

};

#endif