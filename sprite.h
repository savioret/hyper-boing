#ifndef SPRITE_H_
#define SPRITE_H_

#include "pang.h"
//#include <ddraw.h>
//#include "graph.h"

class SPRITE
{
public:
    LPDIRECTDRAWSURFACE7 bmp;
    int sx, sy;
    GRAPH * graph;

    void Init(GRAPH *gr, char file[]);
};

#endif