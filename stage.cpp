#include "pang.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "logger.h"

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
        std::snprintf(back, sizeof(back), "%s", backFile);
}

void Stage::setMusic(const char* musicFile)
{
    if (musicFile)
        std::snprintf(music, sizeof(music), "%s", musicFile);
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
            LOG_DEBUG ( "New object id:%d start:%d x:%d y:%d (%d, %d, %d, %d, %d)", res.id, res.start, res.x, res.y, res.extra.ex1, res.extra.ex2, res.extra.ex3, res.extra.ex4 );
            return res;
        }
    }
    
    return res;
}