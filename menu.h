#ifndef MENU_H_
#define MENU_H_

typedef struct PMNUBITMAPS
{
    SPRITE title;
    SPRITE optionPlay;
    SPRITE optionConfig;
    SPRITE optionExit;
}PMENUBITMAPS;

/*******************************************************
    clase MENU

    Clase Hija de PAPP, es un modulo del juego.

    Es el menu principal del juego.
*********************************************************/
class PMENU : public PAPP
{
public:
    
    PMENUBITMAPS bmp;
    int x, y; // posición del título
    int selectedOption; // 0=JUGAR, 1=CONFIGURACION, 2=SALIR
    BOOL visible; // parpadeo de la opción seleccionada
    int cont; //contador para el parpadeo 
    int Init();
    int InitBitmaps();
    void DrawTitle();
    void DrawMenu();

    void * MoveAll();
    int DrawAll();
    int Release();
};

#endif