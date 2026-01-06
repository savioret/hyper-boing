#ifndef BMFONT_H_
#define BMFONT_H_

/********************************************************
  clase BMNUMFONT

  BitMap Number Font - Es un bitmap que contiene una 
  serie de numeros, del 0 al 9 y por este orden.
  offsets son las posiciones <X> de cada uno, es decir
  donde comienzan.
********************************************************/


class BMNUMFONT
{
public:
    SPRITE *spr;
    int offsets[10];
    int numchars;	
        
    void Init(SPRITE *sprite);
    RECT GetRect( char numchr);
    void SetValue(int number, int xoff);
    void SetValues(int *xoff);

};

#endif