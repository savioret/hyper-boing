/*#include <windows.h>*/
#include <stdio.h>
//#include "sprite.h"
#include "pang.h"




PSCENE::PSCENE(PSTAGE *stg, PSTAGECLEAR *pstgclr)
{
	gameinf.menu = FALSE;
	stage = stg;
	pstageclear = pstgclr;
	if(pstageclear) pstageclear->scene = this;
}

int PSCENE::Init()
{
	char cadena[MAX_PATH];

	change = 0;
	active = TRUE;
	levelclear = FALSE;
	SetGameSpeed(60);
	diftime1=0; 
	diftime2=gamespeed;
	time1 = GetTickCount()+gamespeed;
	time2 = GetTickCount();

	pause = FALSE;
	gameover = FALSE;
	gameovercount = -2;
	timeline = 0;
	dsecond = 0;
	time = stage->timelimit;

	gameinf.player[PLAYER1]->x = stage->xpos[PLAYER1];
	if(gameinf.player[PLAYER2])
		gameinf.player[PLAYER2]->x = stage->xpos[PLAYER2];
	
	CloseMusic();


/*=============================================*/	

	InitBitmaps();
	
	strcpy(cadena, "music\\");
	strcat(cadena, stage->music);
	OpenMusic(cadena);
	PlayMusic();
	return 1;
}

/************************************************************
	  InitBitmaps()
  
	Carga e inicia todos los bitmaps necesarios en este
	modulo del juego (definidos en PSCNBITMAPS).
*************************************************************/
int PSCENE::InitBitmaps()
{	
	int i = 0;
	char cadena[MAX_PATH];

	// offset de las letras en los bitmaps correspondientes
	int offs[10] = {0, 22, 44, 71, 93, 120, 148, 171, 198, 221 };
	int offs1[10] = {0, 13, 18, 31, 44, 58, 70, 82, 93, 105 };
	int offs2[10] = {0, 49, 86, 134, 187, 233, 277, 327, 374, 421 };

	bmp.redball[0].Init(&graph, "graph\\ball-rd1.bmp");
	bmp.redball[1].Init(&graph, "graph\\ball-rd2.bmp");
	bmp.redball[2].Init(&graph, "graph\\ball-rd3.bmp");
	bmp.redball[3].Init(&graph, "graph\\ball-rd4.bmp");
	for(i=0;i<4;i++)
		graph.SetColorKey(bmp.redball[i].bmp, RGB(0,255,0));




	bmp.miniplayer[PLAYER1].Init(&graph, "graph\\miniplayer1.bmp");
	bmp.miniplayer[PLAYER2].Init(&graph, "graph\\miniplayer2.bmp");
	graph.SetColorKey(bmp.miniplayer[PLAYER1].bmp, RGB(0,255,0));
	graph.SetColorKey(bmp.miniplayer[PLAYER2].bmp, RGB(0,255,0));

	bmp.lives[PLAYER1].Init(&graph, "graph\\lives1p.bmp");
	bmp.lives[PLAYER2].Init(&graph, "graph\\lives2p.bmp");
	graph.SetColorKey(bmp.lives[PLAYER1].bmp, RGB(0,255,0));
	graph.SetColorKey(bmp.lives[PLAYER2].bmp, RGB(0,255,0));

	bmp.shoot[0].Init(&graph,"graph\\weapon1.bmp");	
	bmp.shoot[1].Init(&graph,"graph\\weapon2.bmp");
	bmp.shoot[2].Init(&graph,"graph\\weapon3.bmp");
	for(i=0; i<3;i++)
		graph.SetColorKey(bmp.shoot[i].bmp, RGB(0,255,0));

	bmp.mark[0].Init(&graph, "graph\\ladrill1.bmp");
	bmp.mark[1].Init(&graph, "graph\\ladrill1u.bmp");
	bmp.mark[2].Init(&graph, "graph\\ladrill1d.bmp");
	bmp.mark[3].Init(&graph, "graph\\ladrill1l.bmp");
	bmp.mark[4].Init(&graph, "graph\\ladrill1r.bmp");

	for(i=0; i<5;i++)
		graph.SetColorKey(bmp.mark[i].bmp, RGB(0,255,0));

	bmp.floor[0].Init(&graph, "graph\\floor1.bmp");
	graph.SetColorKey(bmp.floor[0].bmp, RGB(0,255,0));
	bmp.floor[1].Init(&graph, "graph\\floor2.bmp");
	graph.SetColorKey(bmp.floor[1].bmp, RGB(0,255,0));

	bmp.time.Init(&graph, "graph\\tiempo.bmp");
	graph.SetColorKey(bmp.time.bmp, RGB(255,0,0));

	bmp.gameover.Init(&graph, "graph\\gameover.bmp", 16, 16);
	graph.SetColorKey(bmp.gameover.bmp, RGB(0,255,0));

	bmp.continu.Init(&graph, "graph\\continue.bmp", 16, 16);
	graph.SetColorKey(bmp.continu.bmp, RGB(0,255,0));

	strcpy(cadena, "graph\\");
	strcat(cadena, stage->back);
	bmp.back.Init(&graph, cadena, 16, 16);
	graph.SetColorKey(bmp.back.bmp, RGB(0,255,0));

	/* inicio de las fuentes numericas */
	bmp.fontnum[0].Init(&graph, "graph\\fontnum1.bmp", 0, 0);
	graph.SetColorKey(bmp.fontnum[0].bmp, RGB(255,0,0));
	fontnum[0].Init(&bmp.fontnum[0]);
	fontnum[0].SetValues(offs);
	bmp.fontnum[1].Init(&graph, "graph\\fontnum2.bmp", 0, 0);
	graph.SetColorKey(bmp.fontnum[1].bmp, RGB(255,0,0));
	fontnum[1].Init(&bmp.fontnum[1]);
	fontnum[1].SetValues(offs1);
	bmp.fontnum[2].Init(&graph, "graph\\fontnum3.bmp", 0, 0);
	graph.SetColorKey(bmp.fontnum[2].bmp, RGB(0,255,0));
	fontnum[2].Init(&bmp.fontnum[2]);
	fontnum[2].SetValues(offs2);


	return 1;
}
/*****************************************************************/
/*******	FUNCIONES DE MODIFICACION Y CONSULTA        **********/
/*****************************************************************/


/************************************************************
	  AddBall()
  
	Añade una Pelota a la lista de pelotas en pantalla
*************************************************************/
void PSCENE::AddBall(int x, int y, int size, int top, int dirx, int diry, int id)
{
	BALL *ball = new BALL(this, x, y, size, top, dirx, diry, id);

	lsballs.Insert((MLISTDATA*) ball);
}

/************************************************************
	  AddItem()
  
	Añade una Pelota a la lista de pelotas en pantalla
	## OPCION NO IMPLEMENTADA ##
*************************************************************/

void PSCENE::AddItem(int x, int y, int id)
{
	ITEM *item = new ITEM(x, y, id);

	lsitems.Insert((MLISTDATA*) item);
}

/************************************************************
	  AddFloor()
  
	Añade un bloque/plataforma a la lista de plataformas
	en pantalla
*************************************************************/
void PSCENE::AddFloor(int x, int y, int id)
{
	FLOOR *floor = new FLOOR(this, x, y, id);
	
	lsfloor.Insert((MLISTDATA*) floor);
}

/************************************************************
	  AddShoot()
  
	Añade un disparo en pantalla, dado un jugador.
*************************************************************/
void PSCENE::AddShoot(PLAYER *pl)
{
	SHOOT *shoot = new SHOOT(this, pl);

	lsshoots.Insert((MLISTDATA*) shoot);
}

/************************************************************
	  Shoot()
  
	Esta funcion comprueba si un jugador puede disparar
	Y en caso afirmativo lo añade a la pantalla.
*************************************************************/
void PSCENE::Shoot(PLAYER *pl)
{
	if (pl->CanShoot())
	{
		pl->Shoot();
		AddShoot(pl);
	}
}

/************************************************************
	  DivideBall()
  
	Crea dos Pelotas a partir de una.
*************************************************************/
int PSCENE::DivideBall(BALL *ball)
{	
	BALL *ball1 ;
	BALL *ball2 ;
	int res = 1;

	if(ball->size < 3)
	{
		ball1 = new BALL(this, ball);
		ball2 = new BALL(this, ball);
		ball1->SetDirX(-1);
		ball2->SetDirX(1);
		
		lsballs.Insert((MLISTDATA*) ball1);
		lsballs.Insert((MLISTDATA*) ball2);		

		res = 0;
	}
	else // la pelota podria ser la ultima ==> WIN
	{
		if(lsballs.GetDimension() == 1 && !stage->itemsleft)
		{
			Win();
		}
	}

	lsballs.DeleteNode(lsballs.Find(ball));
	
	return res;
}

/************************************************************
	  ObjectScore()
  
	Devuelve los puntos asociados a un identificador.
	En nuestro caso usaremos el tamaño de la pelota para
	sumar mas o menos puntos.
*************************************************************/
int PSCENE::ObjectScore(int id)
{
	return 1000/id;
}

/************************************************************
	  Win()
  
	Activa el "flag" de haber superado el nivel, y reproduce
	una musica apropiada.
	Inmediatamente crea una instancia de PSTAGELCEAR para
	mostrar una secuencia.
*************************************************************/
void PSCENE::Win()
{
	CloseMusic();
	OpenMusic("music\\win.mid");
	PlayMusic();
	levelclear = TRUE;

	pstageclear = new PSTAGECLEAR(this);

}


/*****************************************************************/
/*******	FUNCIONES DE OPERACION Y PROCESADO INTERNO  **********/
/*****************************************************************/

/************************************************************
	  CheckColisions()
  
	Comprueba las colisiones de todos los objetos en pantalla
	entre si y actua en consecuencia.
*************************************************************/
void PSCENE::CheckColisions()
{

	MLISTNODE *pt; //puntero para recorrer las listas
	MLISTNODE *ptball = lsballs.GetFirstNode();
	MLISTNODE *ptshoot;
	BALL *b;
	SHOOT *sh;
	FLOOR *fl;
	int i, side = 0;
	POINT col; // colision


	while(ptball) /* COLISIONES DE LA PELOTA */
	{
		b = (BALL*) ptball->data;	

		/* colisiones  PELOTA-DISPARO */
		pt = lsshoots.GetFirstNode();
		while(pt)
		{
			sh = (SHOOT*) pt->data;
			if(!b->hit && !sh->player->dead)
			if(b->Colision(sh))			
			{
				sh->Kill();
				sh->player->LooseShoot();
				sh->player->AddScore(ObjectScore(b->diameter));
				b->Kill();
			}
			pt = lsshoots.GetNextNode(pt);
		}
	
		/* colisiones  PELOTA-PLATAFORMAS */
/*		 Estas colisiones requieren un trato especial
		ya que la pelota puede colisionar con dos puntos
		de la plataforma(superior y izquierdo por ejemplo)
		e incluso con dos plataformas a la vez, lo cual, 
		cuando dos plataformas estan unidas y contiguas
		el comportamiento de la pelota puede no ser el visualmente
		esperado, por ello se deben tener en cuenta todas las
		posibilidades 
*/

		FLOORCOLISION flc[2];
		int cont = 0;
		int moved = 0;

		pt = lsfloor.GetFirstNode();
		while(pt)
		{
			fl = (FLOOR*) pt->data;
			col = b->Colision(fl); //side
			
			if(col.x)
			{
				if(cont) if(flc[0].fl == fl)
				{
					b->SetDirX(-b->dirx);
					moved = 1;
					break;
				}
				if(cont<2)
				{
					flc[cont].col.x = col.x;
					flc[cont].fl = fl;
					cont++;
				}				
			}
			if (col.y)
			{
				if(cont) if(flc[0].fl == fl)
				{
					b->SetDirY(-b->diry);
					moved = 2;
					break;
				}
				if(cont<2)
				{
					flc[cont].col.y = col.y;
					flc[cont].fl = fl;
					cont++;
				}			
			}
			pt = lsfloor.GetNextNode(pt);
		}
		if(cont==1) // solo una colision
		{
			if(flc[0].col.x)
				b->SetDirX(-b->dirx);
			else
				b->SetDirY(-b->diry);		
		}
		else if(cont>1)
		{
			Decide(b, flc, moved);
		}

		/* colisiones  PELOTA-JUGADOR */
		for(i=0;i<2;i++)
		{
			if(gameinf.player[i])
			if(!gameinf.player[i]->immune && !gameinf.player[i]->IsDead())
			{
				if(b->Colision(gameinf.player[i]))
				{
					gameinf.player[i]->Kill();
					gameinf.player[i]->SetFrame(ANIM_DEAD);
				}
			}		
		}

		ptball = lsballs.GetNextNode(ptball);
	}

	ptshoot =lsshoots.GetFirstNode();
	while(ptshoot)
	{
		sh = (SHOOT*) ptshoot->data;
		
		pt = lsfloor.GetFirstNode();
		while(pt)
		{
			fl = (FLOOR*) pt->data;
			if(!sh->dead)
				if(sh->Colision(fl))
				{
					sh->Kill();
					sh->player->LooseShoot();
				}
			pt = lsfloor.GetNextNode(pt);
		}

		ptshoot = lsshoots.GetNextNode(ptshoot);
	}
}


/************************************************************
	  Decide()
  
	Esta es una funciona auxiliar usada para la deteccion de
	colisiones PELOTA-PLATAFORMA.
	Aqui decidimos, cuando hay ambiguedad en la colision, hacia
	que lado rebotara la pelota y hacia que lado no.
*************************************************************/
void PSCENE::Decide(BALL *b, FLOORCOLISION *fc, int moved)
{
	
	// colisiones por el mismo lado en ambos bloques
	if (fc[0].col.x == fc[1].col.x || fc[0].col.y == fc[1].col.y)
	{
		if(fc[0].col.x)  // TOP-DOWN
			if(moved!=1) b->SetDirX(-b->dirx);
		
		if(fc[0].col.y)  // LEFT-RIGHT
				if(moved!=2)b->SetDirY(-b->diry);
		return; 
	}

	// colisiones por lados diferentes
	if(fc[0].fl->id == fc[1].fl->id) // si hay dos bloques concatenados
	{
		if(fc[0].fl->id == 0) // bloque horizontal concatenado			
				if(moved!=2) b->SetDirY(-b->diry); //Solo cambia la Y
		if(fc[0].fl->id == 1) // bloque vertical
				if(moved!=1) b->SetDirX(-b->dirx); // Solo cambia la X
	}
	else // si son dos bloques haciendo esquina
	{	
		if(fc[0].fl->y == fc[1].fl->y)  // los dos al mismo nivel
			if(moved!=2) b->SetDirY(-b->diry); //Solo cambia la Y
		else
			if(moved!=1) b->SetDirX(-b->dirx); // Solo cambia la X
	}

}

/************************************************************
	  CheckSequence()
  
	Aqui se extrae un POBJECT de la secuencia de la pantalla.
	Si la linea de tiempo del juego coincide o es mayor que
	el valor de comienzo de alguno de los objetos contenidos 
	en <stage> entonces se extrae el objeto, y se trata como
	es debido, es decir, se inserta en Pantalla.
*************************************************************/
void PSCENE::CheckSequence()
{
	POBJECT obj;

	do
	{
		obj = stage->Pop(timeline);
		
		// aqui se evaluan los diferentes objetos
		// que podrian aparecer durante el transcurso del nivel
		switch(obj.id)
		{
			case OBJ_BALL:
				if(obj.extra.use)
					AddBall(obj.x, obj.y, obj.extra.ex1, obj.extra.ex2, obj.extra.ex3, obj.extra.ex4, obj.extra.ex5); 
				else
					AddBall(obj.x, obj.y);
			break;
			case OBJ_FLOOR:
				AddFloor(obj.x, obj.y, obj.extra.ex1);
			break;
		}
	}while(obj.id != OBJ_NULL);
}


/************************************************************
	  MoveAll()
  
	Mueve un "fotograma" del juego. Anima el escenario, procesando
	todas las operaciones necesarias.
	Desde aqui es donde se llama a la funcion de Comprobar las
	colisiones, se comprueban estados, se desplazan los objetos
	que lo necesiten, y se lee del teclado para actuar en consecuencia.
*************************************************************/

void * PSCENE::MoveAll()
{	
	MLISTNODE *ptprev, *pt = lsballs.GetFirstNode();	
	BALL *ptb;
	SHOOT *pts;
	FLOOR *pfl;	
	int i, res;

	static int tck=0, lasttck=0, cont = 0;
	tck=GetTickCount();
		if(tck-lasttck>1000)
		{
		  fpsv = cont;
			cont = 0;
			lasttck = tck;
		}
		else
			cont++; //Frames virtuales por segundo

	//input.ReadKeyboard();
	/*gameinf.player[PLAYER1]->score = 100;
	if(input.Key((UINT)'S')) Win();*/
	if(gameover)
	{
		for(i=0;i<2;i++)
		{
			if(gameinf.player[i])
			{				
				if(input.Key(gameinf.keys[i].shoot)) 
				{ 
					if(gameovercount >= 0) // si CONTINUE...
					{
						gameinf.player[PLAYER1]->Init();
						if(gameinf.player[PLAYER2])
							gameinf.player[PLAYER2]->Init();
						gameinf.InitStages();
						return new PSCENE(stage);
					}
					else
					{
						if(gameinf.player[PLAYER1])
						{
							delete gameinf.player[PLAYER1];
							gameinf.player[PLAYER1] = NULL;
						}
						if(gameinf.player[PLAYER2])
						{
							delete gameinf.player[PLAYER2];
							gameinf.player[PLAYER2] = NULL;
						}
						return new PMENU;
					}
				}
			}
		}
	}

	if(!levelclear)
	{
		for(i=0;i<2;i++)
		{
			if(gameinf.player[i])
			{
				if(!gameinf.player[i]->IsDead() && gameinf.player[i]->playing)
					if(gameinf.player[i])
						if (input.Key(gameinf.keys[i].shoot)) {Shoot(gameinf.player[i]);}
						else
							if (input.Key(gameinf.keys[i].left)) gameinf.player[i]->MoveLeft();
							else 
								if (input.Key(gameinf.keys[i].right)) gameinf.player[i]->MoveRight();
								else gameinf.player[i]->Stop();	
				gameinf.player[i]->Update();
			}
		}
		if(gameovercount == -2)
			if(gameinf.player[PLAYER1] && !gameinf.player[PLAYER2])
			{
				if(!gameinf.player[PLAYER1]->playing)
				{
					gameover = TRUE;
					gameovercount = 10;
					CloseMusic();
					OpenMusic("music\\gameover.mid");
					PlayMusic();
				}
			}
			else
				if(!gameinf.player[PLAYER1]->playing && !gameinf.player[PLAYER2]->playing) 
				{
					gameover = TRUE;
					gameovercount = 10;
					CloseMusic();
					OpenMusic("music\\gameover.mid");
					PlayMusic();
				}
	}
	else
	{
		for(i=0;i<2;i++)
			if(gameinf.player[i])
				if(gameinf.player[i]->playing)
					gameinf.player[i]->SetFrame(ANIM_WIN);
	}

	CheckColisions();

	while(pt)
	{
		ptb = (BALL*) pt->data;
		ptb->Move();
		pt = lsballs.GetNextNode(pt);
		if(ptb->hit) DivideBall(ptb);		
	}

	pt = lsshoots.GetFirstNode();
	while(pt)
	{
		pts = (SHOOT*) pt->data;
		pts->Move();
		ptprev = pt; // por si hay que borrar
		pt = lsshoots.GetNextNode(pt);
		if(pts->IsDead())
			lsshoots.DeleteNode(ptprev);
	}

	pt = lsfloor.GetFirstNode();
	while(pt)
	{
		pfl = (FLOOR*) pt->data;
		pfl->Update();		
		pt = lsshoots.GetNextNode(pt);
		/*if(pts->IsDead())
			lsshoots.DeleteNode(ptprev);*/
	}

	
	

	if(dsecond<60) dsecond++; // decimas de segundos
	else  // contador de segundos
	{
		dsecond = 0;
		if(time>0 )
		{
			if(!pstageclear && !gameover) time--;
		}
		else 
		{
			if(!time)
			{
				gameover = TRUE;
				gameovercount = 10;
				gameinf.player[PLAYER1]->playing = FALSE;
				if(gameinf.player[PLAYER2])
					gameinf.player[PLAYER2]->playing = FALSE;
				time = -1;
			}
		}

		timeline++;		
		if(gameover)
		{
			if(gameovercount>=0) gameovercount--;
		} 
	}

	if(pstageclear)
	{
		res =  pstageclear->MoveAll();
		if(res == -1)  // cambiamos de pantalla
		{			
			if(stage->id < gameinf.numstages)
			{
				gameinf.currentstage = stage->id + 1;
				return new PSCENE(&gameinf.stage[stage->id], pstageclear);
			}
			else
				return new PMENU();
		}
		if(res == 0)
		{
			delete pstageclear;
			pstageclear = NULL;
		}
	}
	else CheckSequence();

	return NULL;
}


/************************************************************
	  Release()
  
	Libera los recursos reservados en Init().
*************************************************************/
int PSCENE::Release()
{
	int i;
	lsballs.Release();
	lsshoots.Release();
	lsfloor.Release();

	bmp.back.bmp->Release();
	bmp.floor[0].bmp->Release();
	bmp.floor[1].bmp->Release();
	bmp.fontnum[0].bmp->Release();
	bmp.fontnum[1].bmp->Release();
	bmp.miniplayer[PLAYER1].bmp->Release();
	bmp.miniplayer[PLAYER2].bmp->Release();
	bmp.lives[PLAYER1].bmp->Release();
	bmp.lives[PLAYER2].bmp->Release();
	bmp.time.bmp->Release();
	bmp.gameover.bmp->Release();
	bmp.continu.bmp->Release();
	

	for(i=0;i<5;i++)		
		bmp.mark[i].bmp->Release();
	for(i=0;i<4;i++)		
		bmp.redball[i].bmp->Release();
	for(i=0;i<2;i++)
		bmp.shoot[i].bmp->Release();
	
	CloseMusic();

	delete this;

	return 1 ;
}

/*****************************************************************/
/*******				FUNCIONES DE DIBUJADO            *********/
/*****************************************************************/

void PSCENE::DrawBackground()
{
	graph.Draw(&bmp.back, 0,0);
}

void PSCENE::Draw(BALL *b)
{
	graph.Draw(b->spr, b->x, b->y);
}

void PSCENE::Draw(PLAYER *pl)
{
	graph.Draw(pl->spr, pl->x, pl->y);
}

void PSCENE::Draw(SHOOT *sht)
{
	int i;

	graph.Draw(sht->spr[0], sht->x, sht->y);

	for(i= sht->y + sht->spr[0]->sy ; i< MAX_Y; i+=sht->spr[1]->sy)
		graph.Draw(sht->spr[1+sht->tail], sht->x, i);
/*		
		graph.MoveTo(sht->xi, sht->yi);
	graph.LineTo(sht->x, sht->y);*/
}

void PSCENE::Draw(FLOOR *fl)
{	
	//graph.Rectangle(fl->x, fl->y, fl->x+fl->sx, fl->y+fl->sy);
	graph.Draw(fl->spr, fl->x, fl->y);
}

void PSCENE::DrawScore()
{
	int i;
	if(gameinf.player[PLAYER1]->playing)
	{
		graph.Draw(&fontnum[1], gameinf.player[PLAYER1]->score, 80, RES_Y - 55);	
		graph.Draw(&bmp.miniplayer[PLAYER1], 20, MAX_Y + 7);
		for(i=0;i<gameinf.player[PLAYER1]->lives;i++)
		{
			graph.Draw(&bmp.lives[PLAYER1], 80 + 26*i, MAX_Y + 30);
		}
	}

	if(gameinf.player[PLAYER2])
		if(gameinf.player[PLAYER2]->playing)
		{
			graph.Draw(&bmp.miniplayer[PLAYER2], 400, MAX_Y + 7);
			graph.Draw(&fontnum[1], gameinf.player[PLAYER2]->score, 460, RES_Y - 55);	
			for(i=0;i<gameinf.player[PLAYER2]->lives;i++)
			{
				graph.Draw(&bmp.lives[PLAYER2], 460 + 26*i, MAX_Y + 30);
			}
		}
}

/************************************************************
	  DrawMark()
  
	Dibuja el marco de ladrillo azul.
*************************************************************/

void PSCENE::DrawMark()
{
	int i, j;
	for(i=0, j=0;i<40; i++, j+=16)
	{
		graph.Draw(&bmp.mark[2], j, 0);
		graph.Draw(&bmp.mark[1], j, MAX_Y+1);
		graph.Draw(&bmp.mark[0], j, MAX_Y+17);
		graph.Draw(&bmp.mark[0], j, MAX_Y+33);
		graph.Draw(&bmp.mark[2], j, MAX_Y+49);
	}

	for(i=0, j=0;i<26; i++, j+=16)
	{
		graph.Draw(&bmp.mark[4], 0, j);
		graph.Draw(&bmp.mark[3], MAX_X+1, j);
	}

	graph.Draw(&bmp.mark[0], 0,0);
	graph.Draw(&bmp.mark[0], MAX_X+1, 0);
	graph.Draw(&bmp.mark[0], 0, MAX_Y+1);
	graph.Draw(&bmp.mark[0], MAX_X+1, MAX_Y+1);
}

int PSCENE::DrawAll()
{	
	MLISTNODE *pt = lsballs.GetFirstNode();
	BALL *ptb;
	SHOOT *pts;
	FLOOR *pfl;
	RECT rc;
	HBRUSH hb;
	char cadena[60];
	
	if(!graph.lpDD) return 0;

	DrawBackground();
	graph.GetDC();
	

	/********* VARIABLES A MOSTRAR *********/

	/*	SetBkColor(graph.hdc, RGB(255,255,255));	
	SelectObject(graph.hdc, GetStockObject(BLACK_BRUSH));
	SelectObject(graph.hdc, GetStockObject(BLACK_PEN));
	sprintf(cadena, "FPS = %d FPSVIRT=%d", fps, fpsv);
	graph.Text(cadena, 320, 40);
	sprintf(cadena, "numshoots = %d  cont = %d", gameinf.player[PLAYER1]->numshoots, gameinf.player[PLAYER1]->cont);
	graph.Text(cadena, 320, 60);

	pt = lsballs.GetFirstNode();
	if(pt)
	{
		ptb = (BALL*) pt->data;	
		sprintf(cadena, "y = %f t=%f", ptb->y, ptb->t);	
		graph.Text(cadena, 16, 20);
		//sprintf(cadena, "tmax = %f incy=%d", ptb->tmax, ptb->diry);	
		//graph.Text(cadena, 16, 40);
		//sprintf(cadena, "top = %d y0=%f", ptb->top, ptb->y0);	
		//graph.Text(cadena, 16, 60);
	}	
*/
	
	graph.ReleaseDC();
	
	pt = lsfloor.GetFirstNode();
	while(pt)
	{
		pfl = (FLOOR*) pt->data;
		Draw(pfl);	
		pt = lsshoots.GetNextNode(pt);
	}

	pt = lsshoots.GetFirstNode();
	while(pt)
	{
		pts = (SHOOT*) pt->data;
		Draw(pts);	
		pt = lsshoots.GetNextNode(pt);
	}

	DrawMark();
	DrawScore();
	graph.Draw(&bmp.time, 320-bmp.time.sx/2, MAX_Y +3);
	graph.Draw(&fontnum[FONT_BIG], time, 300, MAX_Y+20);
	
	if(gameinf.player[PLAYER1]->visible && gameinf.player[PLAYER1]->playing)
		Draw(gameinf.player[PLAYER1]);
	
	if(gameinf.player[PLAYER2])
		if(gameinf.player[PLAYER2]->visible && gameinf.player[PLAYER2]->playing)
			Draw(gameinf.player[PLAYER2]);
	
	pt = lsballs.GetFirstNode();
	while(pt)
	{
		ptb = (BALL*) pt->data;
		Draw(ptb);
		pt = lsballs.GetNextNode(pt);
	}

	if(gameover) 
	{
		graph.Draw(&bmp.gameover, 100, 125);
		if(gameovercount>=0)
		{
			graph.Draw(&bmp.continu, 110, 240);
			graph.Draw(&fontnum[FONT_HUGE], gameovercount, 315, 300);
		}
	}



	if(pstageclear) pstageclear->DrawAll();
	graph.Flip();

	static int tck=0, lasttck=0, cont = 0;
	tck=GetTickCount();
		if(tck-lasttck>1000)
		{
		  fps = cont;
			cont = 0;
			lasttck = tck;
		}
		else
			cont++; //Frames por segundo

	return 1;
}

//============================================================//





