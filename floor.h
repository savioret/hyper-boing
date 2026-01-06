#ifndef FLOOR_H_
#define FLOOR_H_


/********************************************************
  clase FLOOR

  Es el objeto "suelo", las plataformas o bloques
  que hay en el aire flotando, que lo unico que hacen es
  molestar la trayectoria de las Bolas.
********************************************************/



class FLOOR
{
public:
    PSCENE *scene;
    SPRITE *spr;

    int x, y;
    int sx, sy;
    int id;
    BOOL dead;

    FLOOR(PSCENE *scn, int _x, int _y, int _id);
    ~FLOOR();

    void Update();
    void Kill(){dead = TRUE ;}
};

#endif