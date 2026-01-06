#ifndef SHOOT_H_
#define SHOOT_H_

/*#include "ball.h"
#include "player.h"*/


/*******************************************************
    clase SHOOT

    El objeto disparo.
*********************************************************/
class SHOOT
{
public:
    PSCENE *scene;
    PLAYER *player; //jugador que lo ha disparado
    SPRITE *spr[3];
    float x, y;
    float xi, yi; // x e Y iniciales
    
    int sx, sy;	

    int id;
    int speed;
    int frame;
    
    int tail;  // esto define la animacion de la cola
    int tailtime;
    int cont; // contador para cambiar la cola


    bool dead;
    
    SHOOT(PSCENE *scn, PLAYER *pl);
    ~SHOOT();

    void Move();
    BOOL Colision(FLOOR *fl);
    void Kill() { dead = 1;}
    bool IsDead() { return dead == 1; }
};

#endif