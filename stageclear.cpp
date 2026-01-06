#include <stdio.h>
#include "pang.h"


PSTAGECLEAR::PSTAGECLEAR(PSCENE *scn)
{
    scene = scn;
    Init();
}

PSTAGECLEAR::~PSTAGECLEAR()
{
    Release();
}

int PSTAGECLEAR::Init()
{
    
    bmp.title1.Init(&graph, "graph\\nivel.png", 0, 0);
    graph.SetColorKey(bmp.title1.bmp, RGB(0,255,0));
    bmp.title2.Init(&graph, "graph\\completado.png", 0, 0);
    graph.SetColorKey(bmp.title2.bmp, RGB(0,255,0));
    bmp.roof.Init(&graph, "graph\\ladrill4.png", 0, 0);
    graph.SetColorKey(bmp.roof.bmp, RGB(0,255,0));

    xt1 = - bmp.title1.sx;
    xt2 = 640;
    yt1 = 50;
    yt2 = 50 + bmp.title1.sy + scene->bmp.fontnum[FONT_HUGE].sy +25;
    yr1 = -16;
    yr2 = 480;

    xnum = 275;
    ynum = -90;

    cscore[PLAYER1] = 0;
    cscore[PLAYER2] = 0;

    endmove = FALSE;
    endcount = FALSE;
    movingout = FALSE;
    finish = FALSE;
    closing = opening = FALSE;
    endclose = endopening = FALSE;

    return 1;
}

void PSTAGECLEAR::DrawAll()
{
    int i,j;
    char cad[10 ];
    
    sprintf(cad, "%2d", gameinf.currentstage);
    for(i=0;i<strlen(cad);i++)
        if(cad[i] == ' ') cad[i] = '0';

    if(closing)
    {			
        for(i=-16;i<(yr1/16)+1;i++)
            for(j=0;j<40;j++)
            {	
                graph.DrawClipped(&bmp.roof, j*16, /**/+i*16-(i%16));
                graph.DrawClipped(&bmp.roof, j*16, 480-(i*16));
            }
    }	
    else
        if(opening)
        {			
            for(i=(yr1/16)+1;i>-17;i--)
                for(j=0;j<40;j++)
                {	
                    graph.DrawClipped(&bmp.roof, j*16, /**/+i*16-(i%16));
                    graph.DrawClipped(&bmp.roof, j*16, 480-(i*16));
                }
        }	

    graph.DrawClipped(&bmp.title1, xt1, yt1);
    graph.DrawClipped(&bmp.title2, xt2, yt2);
    graph.DrawClipped(&scene->fontnum[FONT_HUGE], cad, xnum, ynum);
    
    
    if(finish) return;
    if(!closing)
    {
        if(gameinf.player[PLAYER1]->playing)
            graph.Draw(&scene->bmp.miniplayer[PLAYER1], 40, 300);
        if(gameinf.player[PLAYER2])
            if(gameinf.player[PLAYER2]->playing)
                graph.Draw(&scene->bmp.miniplayer[PLAYER2], 350, 300);
    }

    if(endmove && !closing)
    {
        if(gameinf.player[PLAYER1]->playing)
            graph.Draw(&scene->fontnum[FONT_SMALL], cscore[PLAYER1], 105, 320);
        if(gameinf.player[PLAYER2])
            if(gameinf.player[PLAYER2]->playing)
                graph.Draw(&scene->fontnum[FONT_SMALL], cscore[PLAYER2], 450, 320);
    }
                
}

int PSTAGECLEAR::MoveAll()
{
    BOOL a = FALSE, b = FALSE, c = FALSE;

    //input.ReadKeyboard();
    if(input.Key(gameinf.keys[PLAYER1].shoot))
    {
        if(!endcount)
        {
            endcount = TRUE;
            cscore[PLAYER1] = gameinf.player[PLAYER1]->score;
            if(gameinf.player[PLAYER2])
                cscore[PLAYER2] = gameinf.player[PLAYER2]->score;
        }
        closing = TRUE;

    }
    else
        if(gameinf.player[PLAYER2])
            if(input.Key(gameinf.keys[PLAYER2].shoot))
            {
                if(!endcount)
                {
                    endcount = TRUE;				
                    cscore[PLAYER1] = gameinf.player[PLAYER1]->score;
                    if(gameinf.player[PLAYER2])
                        cscore[PLAYER2] = gameinf.player[PLAYER2]->score;
                }
                closing = TRUE;
            }

    if(closing)
    {
        if(yr1<240) yr1+=4;
        if(yr2>241) yr2-=4;
        else 
        {
            endclose = TRUE;
            //closing = FALSE;			
            movingout = TRUE;
            
        }
    }
    else
        if(opening)
        {
            if(yr1>-32) yr1-=4;
            if(yr2<481) yr2+=4;
            else 
            {
                endopening = TRUE;
                opening = FALSE;
                movingout = TRUE;
                return 0;
            }
        }

    if(movingout)  // las letras que se van
    {
        if(xt1 < 640)
            xt1 += 4;
        else a = TRUE;

        if(xt2 > -bmp.title2.sx)
            xt2 -= 5;
        else b = TRUE;

        if(ynum < 480 )
            ynum += 5;
        else c = TRUE;

        if(a&&b&&c)
        {
            finish = TRUE;
            opening = TRUE;
            closing = FALSE;
            movingout = FALSE;
            return -1;
        }
    }

    if(!endmove)
    {
        if(xt1 < 250)
            xt1 += 4;
        else a = TRUE;

        if(xt2 > 175)
            xt2 -= 5;
        else b = TRUE;

        if(ynum < 100)
            ynum += 3;
        else c = TRUE;

        if(a&&b&&c) endmove = TRUE;
    }
    else
        if(!endcount)
        {
            cscore[PLAYER1]++;			

            if(gameinf.player[PLAYER2])
            {
                if(cscore[PLAYER2] < gameinf.player[PLAYER2]->score)
                    cscore[PLAYER2]++;
                if(cscore[PLAYER1] >= gameinf.player[PLAYER1]->score &&
                    cscore[PLAYER2] >= gameinf.player[PLAYER2]->score)
                {
                    endcount = TRUE;
                    closing = TRUE;
                }
            }
            else 
                if(cscore[PLAYER1] >= gameinf.player[PLAYER1]->score)
                {
                    endcount = TRUE;
                    closing = TRUE;
                }
        }

        return 1;
}

int PSTAGECLEAR::Release()
{
    bmp.title1.Release();
    bmp.title2.Release();

        return 1;
}
