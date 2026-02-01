#include "main.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <climits>
#include "logger.h"

void Stage::reset()
{
    sequence.clear();  // unique_ptr automatically deletes
    sequenceIndex = 0;
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

void Stage::spawn(StageObject&& obj)
{
    // Create unique_ptr and add to vector
    sequence.push_back(std::make_unique<StageObject>(std::move(obj)));
    
    // Get reference to the object we just added
    auto& newObj = sequence.back();
    
    // Simply copy coordinates from params (no positioning logic here)
    if (newObj->params)
    {
        newObj->x = newObj->params->x;
        newObj->y = newObj->params->y;
    }
    
    if (newObj->id == OBJ_BALL)
        itemsleft++;
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

    // Check if we have more objects to pop
    if (sequenceIndex < sequence.size())
    {
        const auto& obj = sequence[sequenceIndex];
        
        if (time >= obj->start)
        {
            // Move ownership out of the unique_ptr
            res = std::move(*obj);
            
            // Update itemsleft before advancing
            if (res.id == OBJ_BALL) itemsleft--;
            
            // Advance to next object
            sequenceIndex++;
            
            // Keep INT_MAX values intact - Scene will handle random position calculation
            // Update params to reflect current position (may still be INT_MAX)
            if (res.params)
            {
                res.params->x = res.x;
                res.params->y = res.y;
            }
            
            // Enhanced logging with typed params
            if (res.params)
            {
                if (auto* ball = res.getParams<BallParams>())
                {
                    LOG_DEBUG("Pop BALL id:%d start:%d x:%d y:%d size:%d top:%d dirX:%d dirY:%d type:%d",
                        res.id, res.start, res.x, res.y,
                        ball->size, ball->top, ball->dirX, ball->dirY, ball->ballType);
                }
                else if (auto* floor = res.getParams<FloorParams>())
                {
                    LOG_DEBUG("Pop FLOOR id:%d start:%d x:%d y:%d floorType:%d",
                        res.id, res.start, res.x, res.y, floor->floorType);
                }
                else
                {
                    LOG_DEBUG("Pop object id:%d start:%d x:%d y:%d (unknown params type)",
                        res.id, res.start, res.x, res.y);
                }
            }
            else
            {
                LOG_DEBUG("Pop object id:%d start:%d x:%d y:%d (no params)",
                    res.id, res.start, res.x, res.y);
            }
        }
    }
    
    return res;
}