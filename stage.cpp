#include "pang.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

void Stage::reset()
{
    for (StageObject* obj : sequence)
    {
        delete obj;
    }
    sequence.clear();
    itemsleft = 0;
    id = 0;
}

void Stage::setBack(const char* backFile)
{
    if (backFile)
        std::strncpy(back, backFile, sizeof(back) - 1);
}

void Stage::setMusic(const char* musicFile)
{
    if (musicFile)
        std::strncpy(music, musicFile, sizeof(music) - 1);
}

void Stage::add(int idObject, int start)
{
    StageObject* obj = new StageObject(idObject, start);

    obj->x = std::rand() % 600 + 32;
    obj->y = 22;

    sequence.push_back(obj);

    if (obj->id == OBJ_BALL)
        itemsleft++;
}

void Stage::addX(int idObject, int start, StageExtra extra)
{
    StageObject* obj = new StageObject(idObject, start, extra);

    obj->x = std::rand() % 600 + 32;
    obj->y = 22;
    sequence.push_back(obj);

    if (obj->id == OBJ_BALL)
        itemsleft++;
}

void Stage::add(int idObject, int x, int y, int start)
{
    StageObject* obj = new StageObject(idObject, start);
    obj->x = x;
    obj->y = y;

    sequence.push_back(obj);

    if (obj->id == OBJ_BALL)
        itemsleft++;
}

void Stage::addX(int idObject, int x, int y, int start, StageExtra extra)
{
    StageObject* obj = new StageObject(idObject, start, extra);
    obj->x = x;
    obj->y = y;

    sequence.push_back(obj);

    if (obj->id == OBJ_BALL)
        itemsleft++;
}

StageObject Stage::pop(int time)
{
    StageObject res(OBJ_NULL);

    if (!sequence.empty())
    {
        StageObject* obj = sequence.front();
        if (time >= obj->start)
        {
            res = *obj;
            delete obj;
            sequence.pop_front();
            if (res.id == OBJ_BALL) itemsleft--;			
            return res;
        }
    }
    
    return res;
}