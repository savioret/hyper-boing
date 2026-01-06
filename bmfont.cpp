#include "pang.h"
//#include "bmfont.h"

void BMNUMFONT::Init(SPRITE *sprite)
{
    spr = sprite;

    numchars = 10;
}

void BMNUMFONT::SetValue(int number, int xoff)
{
    if(number >= 0 && number <=9 )
        offsets[number] = xoff;
}

void BMNUMFONT::SetValues(int *xoff)
{
    int i;

    for(i=0; i<numchars;i++)	
        offsets[i] = xoff[i];
}

RECT BMNUMFONT::GetRect( char numchr)
{
    RECT rcchr;
    int code;

    /* los codigos ascii van del 48 al 57 */
    code = numchr;
    code -= 48;

    if(code >= 0 && code <= 9)
    {
        rcchr.top = 0;
        rcchr.bottom = spr->sy;
        rcchr.left = offsets[code];
        if(code == numchars-1) rcchr.right = spr->sx - 1;
            else rcchr.right = offsets[code + 1];		
    }
    else
        SetRect(&rcchr, 0,0,0,0);

    return rcchr;
}