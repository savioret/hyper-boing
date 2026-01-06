#ifndef STAGE_H_
#define STAGE_H_


/************************************************************
    clase PEXTRA
  Esta clase se usa como parametro, donde habra informacion extra
  acerca del POBJECT que tratemos, y cada campo tendra una informacion
  con un objetivo diferente dependiendo del objeto que se trate:

  OBJETO BALL
    ex1 = tamanyo (0=max  3=min)
    ex2 = valor <top> del objeto
    ex3 = dirx
    ex4 = diry
    ex5 = tipo de BALL

  OBJETO FLOOR
    ex1 = tipo de FLOOR
*************************************************************/

class PEXTRA
{
public:
    BOOL use;
    int ex1;
    int ex2;
    int ex3;
    int ex4;
    int ex5;

    PEXTRA()
    { 
        use=ex1=ex2=ex3=ex5=0;
    }
};

/************************************************************
    clase POBJECT
  Esta clase estará a su vez incluida en la clase PSTAGE
  explicada a continuacion.
  Aqui se guarda informacion del objeto, posicion, identificador
   y cuando aparecera en pantalla.(ver PSTAGE abajo)

*************************************************************/

typedef struct POBJECT
{
    int id;
    int start; // tiempo de comienzo
    int x, y;
    PEXTRA extra; //informacion extra, su uso dependera de cada objeto

    POBJECT(int _id, int _start=0)
    {
        id=_id;
        start = _start;	
        x=y=0;
        extra.use = FALSE;
    }

    POBJECT(int _id, int _start, PEXTRA _extra)
    {
        id=_id;
        start = _start;
        extra = _extra;
        extra.use = TRUE;
        x=y=0;
    }

    POBJECT()
    {
        extra.use = FALSE;
    }
};

/************************************************************
      clase PSTAGE
  
    Esta clase es un nivel del juego. Continene informacion
    del numero de la pantalla, la imagen de fondo, la musica, 
    el tiempo limite, etc.

    Ademas una lista enlazada con todos los POBJECTS que iran
    apareciendo a lo largo de la partida, dependiendo de la
    linea de tiempo (definida en la PSCENE).
*************************************************************/

class PSTAGE
{
public:
    int id; // ID de la pantalla
    char back[64]; /* background */
    char music[64]; /*musica ambiente */
    int timelimit;
    int itemsleft; // elementos "clave" que quedan por aparecer
    int xpos[2]; //posicion inicial para jugador 1 y 2
                   
private:
    MLIST sequence;

public:
    PSTAGE(){ itemsleft=0;}


    void Reset();
    void SetBack(char _back[]);
    void SetMusic(char _music[]);
    void Add(int idobject, int start);
    void AddX(int idobject, int start, PEXTRA extra);
    void Add(int idobject, int x, int y,  int start);
    void AddX(int idobject, int x, int y,  int start, PEXTRA extra);

    POBJECT Pop(int time);
};

#endif