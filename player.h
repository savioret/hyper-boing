#ifndef _PLAYER_H_
#define _PLAYER_H_


class PSCENE;


/*******************************************************
    clase PLAYER

    jugador, como su nombre indica, contiene informacion
    sobre el sprite que identifica al jugador, las teclas
    de movimiento y accion, la puntuacion, las vidas, 
    armas, e informacion para gestion interna.
*********************************************************/

class PLAYER
{
public:
    SPRITE * spr;
    float x, y;
    int sx, sy;
    int xdir, ydir; //esto se usa para la animacion de muerto
    int lives;
    int score;
    int id;  // 0 = jugador1   1=jugador2
    int idweapon;
    int maxshoots;
    int numshoots;
    int cont; // contador interno para los disparos
    int interval; //intervalo de tiempo entre disparo y disparo
    int speed; //velocidad de la animacion
    int conts; //contador para la velocidad de la animacion
    int inc; //incremento de desplazamiento al andar
    int frame;
    BOOL dead;
    BOOL playing;
    int immune; // para cuando te acaban de matar
    BOOL visible;

    PLAYER(int _sx, int _sy, int _id);
    PLAYER(int _id);
    ~PLAYER();
    void Init();
    void SetFrame(int _frame);

    void Update();
    void AddScore(int num);
    void MoveLeft();
    void MoveRight();
    void Stop();
    BOOL CanShoot();
    BOOL IsDead(){return dead;}
    void Shoot();
    void LooseShoot();
    void Kill();
    void Revive();

};

#endif