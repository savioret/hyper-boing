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

// NEW: Type-safe spawn methods

void Stage::spawn(StageObject obj)
{
    StageObject* newObj = new StageObject(std::move(obj));
    
    // Apply spawn mode positioning
    if (newObj->params)
    {
        switch (newObj->params->spawnMode)
        {
        case SpawnMode::FIXED:
            newObj->x = newObj->params->x;
            newObj->y = newObj->params->y;
            break;
            
        case SpawnMode::RANDOM_X:
            newObj->x = std::rand() % 600 + 32;
            newObj->y = newObj->params->y;
            break;
            
        case SpawnMode::TOP_RANDOM:
            newObj->x = std::rand() % 600 + 32;
            newObj->y = 22;
            break;
        }
        
        // Update params to reflect actual position
        newObj->params->x = newObj->x;
        newObj->params->y = newObj->y;
    }
    
    sequence.push_back(newObj);
    
    if (newObj->id == OBJ_BALL)
        itemsleft++;
}

void Stage::spawnBallAtTop(int startTime, int size, int dirX, int dirY)
{
    BallParams params;
    params.startTime = startTime;
    params.size = size;
    params.dirX = dirX;
    params.dirY = dirY;
    params.spawnMode = SpawnMode::TOP_RANDOM;
    
    spawnBall(params);
}

void Stage::spawnBall(const BallParams& params)
{
    auto paramsCopy = std::make_unique<BallParams>(params);
    spawn(StageObject(OBJ_BALL, std::move(paramsCopy)));
}

void Stage::spawnFloor(const FloorParams& params)
{
    auto paramsCopy = std::make_unique<FloorParams>(params);
    spawn(StageObject(OBJ_FLOOR, std::move(paramsCopy)));
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
            
            // Enhanced logging with typed params
            if (res.params)
            {
                if (auto* ball = res.getParams<BallParams>())
                {
                    LOG_DEBUG("New BALL id:%d start:%d x:%d y:%d size:%d top:%d dirX:%d dirY:%d type:%d",
                        res.id, res.start, res.x, res.y,
                        ball->size, ball->top, ball->dirX, ball->dirY, ball->ballType);
                }
                else if (auto* floor = res.getParams<FloorParams>())
                {
                    LOG_DEBUG("New FLOOR id:%d start:%d x:%d y:%d floorType:%d",
                        res.id, res.start, res.x, res.y, floor->floorType);
                }
                else
                {
                    LOG_DEBUG("New object id:%d start:%d x:%d y:%d (unknown params type)",
                        res.id, res.start, res.x, res.y);
                }
            }
            else
            {
                LOG_DEBUG("New object id:%d start:%d x:%d y:%d (no params)",
                    res.id, res.start, res.x, res.y);
            }
            
            return res;
        }
    }
    
    return res;
}