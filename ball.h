#ifndef _BALL_H_
#define _BALL_H_

class PSCENE;
class SHOOT;
class PLAYER;
class FLOOR;

/********************************************************
  clase BALL

  es la pelota del juego, a la que hay que disparar
********************************************************/


class BALL
{
public:

    SPRITE * spr;
    float x, y; // posicion esquina superior izq.
    int top; //altura maxima desde el suelo
    int diameter;  //diametro
    int size;
    int id;

    int dirx, diry; //direccion
    float t, tmax;	// tiempo	
    float y0; //espacio inicial
    float acc;
    PSCENE *scene;
    BOOL hit;

    BALL(PSCENE *scn, BALL *oldball);
    BALL(PSCENE *scn, int _x, int _y, int _size,  int _dirx=1, int _diry=1, int _top=0, int _id=0);
    ~BALL();
    void Init();
    void InitTop();
    
    void Move();
    void Kill();
    BOOL Colision(SHOOT * sh);
    POINT Colision(FLOOR * fl);
    BOOL Colision(PLAYER *pl);
    void SetDir(int _dirx, int _diry);
    void SetDirX(int _dirx);
    void SetDirY(int _diry);
    void SetPos(int x, int y);
    
};


#endif