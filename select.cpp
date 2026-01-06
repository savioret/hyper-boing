#include "pang.h"

int PSELECT::InitBitmaps()
{
    bmp.back.Init(&graph, "graph\\titleback.png", 0, 0);
    graph.SetColorKey(bmp.back.bmp, RGB(255,0,0));

    bmp.mode.Init(&graph, "graph\\selecmodo.png", 0, 0);
    graph.SetColorKey(bmp.mode.bmp, RGB(255,0,0));
    
    bmp.seltext[PLAYER1].Init(&graph, "graph\\select1ptext.png", 0, 0);
    graph.SetColorKey(bmp.seltext[PLAYER1].bmp, RGB(0,0,255));
    bmp.seltext[PLAYER2].Init(&graph, "graph\\select2ptext.png", 0, 0);
    graph.SetColorKey(bmp.seltext[PLAYER2].bmp, RGB(0,0,255));

    bmp.select[PLAYER1].Init(&graph, "graph\\select1p.png", 0, 0);
    graph.SetColorKey(bmp.select[PLAYER1].bmp, RGB(0,255,0));
    bmp.select[PLAYER2].Init(&graph, "graph\\select2p.png", 0, 0);
    graph.SetColorKey(bmp.select[PLAYER2].bmp, RGB(150,150,150));

    return 1;
}


int PSELECT::Init()
{
    gameinf.menu = FALSE;
    InitBitmaps();
    xb=yb=0;
    yb = bmp.back.sy;

    option = 0;
    initdelay = 40;

    SetGameSpeed(GLOBAL_GAMESPEED);
    diftime1=0; 
    diftime2=gamespeed;
    time1 = GetTickCount()+gamespeed;
    time2 = GetTickCount();

    delay = 13;
    count = 0;

    pause = FALSE;	

    return 1;
}

int PSELECT::Release()
{
    bmp.select[PLAYER1].Release();
    bmp.select[PLAYER2].Release();
    bmp.seltext[PLAYER1].Release();
    bmp.seltext[PLAYER2].Release();
    bmp.mode.Release();
    bmp.back.Release();

    CloseMusic();

    return 1;
}

void PSELECT::DrawSelect()
{
    //graph.GetDC();
    //SelectObject(graph.hdc, GetStockObject(WHITE_BRUSH));
    if (option==0) 	
        graph.Rectangle(65, 205, 70+bmp.select[PLAYER1].sx+5, 210 + bmp.select[PLAYER1].sy+5);
    else
        graph.Rectangle(330, 205, 335+bmp.select[PLAYER2].sx+5, 210 + bmp.select[PLAYER2].sy+5);


    //graph.ReleaseDC();
    graph.Draw(&bmp.mode, 38, 10);
    graph.Draw(&bmp.select[PLAYER1], 70, 210);
    graph.Draw(&bmp.seltext[PLAYER1], 70, 210+bmp.select[PLAYER1].sy + 10);
    graph.Draw(&bmp.select[PLAYER2], 335, 210);
    graph.Draw(&bmp.seltext[PLAYER2], 350 , 210+bmp.select[PLAYER2].sy + 10);
}

void PSELECT::DrawBack()
{
    int i, j;
    RECT rc, rcbx, rcby, rcq;	
    
    rc.left = xb;
    rc.right = bmp.back.sx;
    rc.top = 0;
    rc.bottom = yb;
    

    rcbx.left = 0;
    rcbx.right = xb;
    rcbx.top = 0;
    rcbx.bottom = yb;

    rcby.left = xb;
    rcby.right = bmp.back.sx;
    rcby.top = yb;
    rcby.bottom = bmp.back.sy;

    rcq.left = 0;
    rcq.right = xb;
    rcq.top = yb;
    rcq.bottom = bmp.back.sy;
    

    for(i=0;i<4;i++)
        for(j=0;j<5;j++)
        {
            graph.Draw(bmp.back.bmp, &rc, bmp.back.sx*i, (bmp.back.sy*j)+bmp.back.sy-yb);
            graph.Draw(bmp.back.bmp, &rcbx, (bmp.back.sx*i)+rc.right-rc.left, (bmp.back.sy*j)+bmp.back.sy - yb);
            graph.Draw(bmp.back.bmp, &rcby, bmp.back.sx*i, bmp.back.sy*j);
            graph.Draw(bmp.back.bmp, &rcq, (bmp.back.sx*i)+bmp.back.sx-xb, bmp.back.sy*j);
        }
}

int PSELECT::DrawAll()
{
    
    DrawBack();
    DrawSelect();

    graph.Flip();
    
    return 1;
}

void * PSELECT::MoveAll()
{
    if(xb< bmp.back.sx) xb++;
    else xb = 0;

    if(yb>0) yb--;
    else yb = bmp.back.sy;

    if(initdelay>0) initdelay--;

    //input.ReadKeyboard();

    if(input.Key( SDL_SCANCODE_ESCAPE ))
        return new PMENU;

    
    if(input.Key(gameinf.keys[PLAYER1].left) || input.Key(gameinf.keys[PLAYER1].right))
    {
        if(!count)
        {
            option = !option;
            count = delay;
        }
    }
    else
        if(!initdelay)
            if(input.Key(gameinf.keys[PLAYER1].shoot))
            {
                gameinf.player[PLAYER1] = new PLAYER(PLAYER1);
                if(option == PLAYER2)
                        gameinf.player[PLAYER2] = new PLAYER(PLAYER2);
                gameinf.InitStages();
                return new PSCENE(&gameinf.stage[0]);
            }

    if(count>0) count--;	

    return NULL;
}