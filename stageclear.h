#ifndef STAGECLEAR_H_
#define STAGECLEAR_H_


class PSCENE;


typedef struct STAGECLEARBITMAPS
{
    SPRITE title1;
    SPRITE title2;
    SPRITE roof;

}PSTGCLRBITMAPS;


/*******************************************************
    clase PSTAGECLEAR

    Clase contenida en PSCENE(el modulo del juego).
    El objeto creado en PSCENE es un puntero a NULL que
    se inicializa en el momento que haga falta, y se destruye
    del mismo modo.

  El motivo de crear esta clase era para no hacer tan denso
  el codigo y hacer que quedase un poco mas legible dentro de
  PSCENE, ya que esta secuencia requiere muchos contadores
  y variables de estados.
*********************************************************/
class PSTAGECLEAR
{
public:
    
    PSTGCLRBITMAPS bmp;
    PSCENE *scene;

    int xt1, yt1;
    int xt2, yt2;
    int xnum, ynum;

    int yr1, yr2;

    int cscore[2];

    BOOL endmove;
    BOOL endcount;
    BOOL endclose;
    BOOL endopening;
    BOOL movingout;
    BOOL closing;
    BOOL opening;
    BOOL finish;

    PSTAGECLEAR(PSCENE *scn);
    ~PSTAGECLEAR();
    void DrawAll();
    int MoveAll();
    int Init();
    int Release();
};

#endif