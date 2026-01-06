#ifndef SELECT_H_
#define SELECT_H_

typedef struct PSLCBITMAPS
{
    SPRITE back;
    SPRITE select[2];
    SPRITE seltext[2];
    SPRITE mode;
}PSELECTBITMAPS;

/*******************************************************
    clase PSELECT

    Clase Hija de PAPP, es un modulo del juego.

    Es el menu de seleccion de numero de jugadores.
*********************************************************/
class PSELECT : public PAPP
{
public:
    
    PSELECTBITMAPS bmp;
    int xb, yb; //x e y del background
    int option;

    int delay; // tiempo entre el cambio de una opcion a otra
    int count; // contador para el delay
    int initdelay; // para que no pulse directamente
    
    int Init();
    int InitBitmaps();
    void DrawSelect();
    void DrawBack();	

    void * MoveAll();
    int DrawAll();
    int Release();
};

#endif