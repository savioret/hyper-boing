#include "pang.h"


/************************************************************
      Reset()
  
    Reinicia la secuencia.
*************************************************************/

void PSTAGE::Reset()
{
    sequence.Release();
    itemsleft = 0;
    id = 0;
}

/************************************************************
      SetBack()
  
    cadena de caracteres conteniendo el BackGround.
*************************************************************/
void PSTAGE::SetBack(char _back[])
{
    strcpy(back, _back);
}

void PSTAGE::SetMusic(char _music[])
{
    strcpy(music, _music);
}

/************************************************************
      Add()
  
    Añade un objeto a la secuencia, con un tiempo de salida
    determinado.
    Puesto que las coordenadas del objeto no estan indicadas
    la coordenada X es aleatoria y la coordenada Y es la altura
    maxima.
*************************************************************/
void PSTAGE::Add(int idobject, int start)
{
    POBJECT *obj = new POBJECT(idobject, start);

    obj->x = rand()%600+32;
    obj->y = 22;

    sequence.Insert((MLISTDATA*) obj);

    if(obj->id == OBJ_BALL)
        itemsleft++;
}

/************************************************************
      AddX()
  
    Añade un objeto con propiedades *extra*
    Para mas informacion ver PEXTRA
*************************************************************/

void PSTAGE::AddX(int idobject, int start, PEXTRA extra)
{
    POBJECT *obj = new POBJECT(idobject, start, extra);

    obj->x = rand()%600+32;
    obj->y = 22;
    sequence.Insert((MLISTDATA*) obj);

    if(obj->id == OBJ_BALL)
        itemsleft++;
}


void PSTAGE::Add(int idobject, int x, int y,  int start)
{
    POBJECT *obj = new POBJECT(idobject, start);
    obj->x = x;
    obj->y = y;

    sequence.Insert((MLISTDATA*) obj);

    if(obj->id == OBJ_BALL)
        itemsleft++;
}

void PSTAGE::AddX(int idobject, int x, int y,  int start, PEXTRA extra)
{
    POBJECT *obj = new POBJECT(idobject, start, extra);
    obj->x = x;
    obj->y = y;

    sequence.Insert((MLISTDATA*) obj);

    if(obj->id == OBJ_BALL)
        itemsleft++;
}

/************************************************************
      Pop()
  
    Comprueba la linea de tiempo(time) y comprueba si el parametro
    start del primer objeto de la lista es menor o igual que time.
    En ese caso el valor devuelto por POP es el objeto en cuestion
    y se elimina de la lista.
    Si este objeto no cumple la propiedad entonces se devuelve 
    el objeto con id=OBJ_NULL.
*************************************************************/
POBJECT PSTAGE::Pop(int time)
{
    MLISTNODE *pt = sequence.GetFirstNode();	
    POBJECT *obj;
    POBJECT res;

    if(pt)
    {
        obj = (POBJECT*) pt->data;
        if(time >= obj->start)
        {
            res = *obj;
            sequence.DeleteNode(pt);
            if(res.id == OBJ_BALL) itemsleft--;			
            return res;
        }
    }
    
    res.id = OBJ_NULL;
    return res;
}