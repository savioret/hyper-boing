#include "pang.h"
#include <stdio.h>

int PMENU::InitBitmaps()
{
	bmp.title.Init(&graph, "graph\\title.bmp", 0, 0);
	graph.SetColorKey(bmp.title.bmp, RGB(0,255,0));
	bmp.back.Init(&graph, "graph\\titleback.bmp", 0, 0);
	graph.SetColorKey(bmp.back.bmp, RGB(255,0,0));
	bmp.pulsa.Init(&graph, "graph\\pulsatecla.bmp", 0, 0);
	graph.SetColorKey(bmp.pulsa.bmp, RGB(255,0,0));

	return 1;
}


int PMENU::Init()
{
	gameinf.menu = TRUE;
	InitBitmaps();
	xb=yb=0;
	x = 80;
	y = -500;

	yb = bmp.back.sy;
	
	SetGameSpeed(60);
	diftime1=0; 
	diftime2=gamespeed;
	time1 = GetTickCount()+gamespeed;
	time2 = GetTickCount();

	pause = FALSE;
	visible = FALSE;
	cont = 30;

	CloseMusic();
	OpenMusic("music\\menu.mid");
	PlayMusic();

	return 1;
}

int PMENU::Release()
{
	bmp.title.bmp->Release();
	bmp.back.bmp->Release();
	bmp.pulsa.bmp->Release();

	CloseMusic();

	return 1;
}

void PMENU::DrawTitle()
{
	int i, j;

	graph.DrawClipped(&bmp.title, x, y);
}

void PMENU::DrawBack()
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

int PMENU::DrawAll()
{
	char cadena[100];
	DrawBack();
	DrawTitle();
	if(visible)
		graph.Draw(&bmp.pulsa, 170, 350);

		
	
	/******** Copyright *********/
	graph.GetDC();
	SetBkColor(graph.hdc, RGB(0,0, 0));
	SetTextColor(graph.hdc, RGB(255,255,255));
	SelectObject(graph.hdc, GetStockObject(WHITE_BRUSH));
	SelectObject(graph.hdc, GetStockObject(WHITE_PEN));

	SetBkMode(graph.hdc, TRANSPARENT);		
	//SetBkColor(graph.hdc, RGB(255,255, 255));
	sprintf(cadena, "Chuper Pang © Miguel Martinez, 2001");
	graph.Text(cadena, 100, RES_Y-20);			
	graph.ReleaseDC();

	graph.Flip();
	
	return 1;
}

void * PMENU::MoveAll()
{

	if(cont>0) cont--;
	else cont = 30;

	if(xb< bmp.back.sx) xb++;
	else xb = 0;

	if(yb>0) yb--;
	else yb = bmp.back.sy;

	if(y<50) y+=8;
	else
		if(!cont) visible = !visible;

	//input.ReadKeyboard();

	if(input.Key(gameinf.keys[PLAYER1].shoot))
			return new PSELECT;
	
	return NULL;
}