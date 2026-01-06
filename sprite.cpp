#include "pang.h"

void SPRITE::Init(GRAPH *gr, char file[])
{
    DDSURFACEDESC2 ddsd;
    graph = gr;
    graph->LoadBitmap( file, 0, 0);

    bmp->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    sx = ddsd.dwWidth;
    sy = ddsd.dwHeight;
    bmp->Unlock(NULL);
}


