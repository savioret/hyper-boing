#include "main.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <climits>
#include "logger.h"

void Stage::reset()
{
    sequence.clear();  // unique_ptr automatically deletes
    //itemsleft = 0;
    restart();
}

void Stage::restart()
{
    sequenceIndex = 0;
    id = 0;
    countItemsLeft();
}

void Stage::countItemsLeft()
{
    itemsleft = 0;

    // Count all Ball and Hexa objects in the sequence
    for (const auto& obj : sequence)
    {
        if (obj && (obj->id == StageObjectType::Ball || obj->id == StageObjectType::Hexa))
        {
            itemsleft++;
        }
    }

    LOG_DEBUG("Stage %d: Counted %d balls/hexas in sequence", id, itemsleft);
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
    
    if (newObj->id == StageObjectType::Ball || newObj->id == StageObjectType::Hexa)
        itemsleft++;
}

void Stage::spawnBall(const BallParams& params)
{
    auto paramsCopy = std::make_unique<BallParams>(params);
    spawn(StageObject(StageObjectType::Ball, std::move(paramsCopy)));
}

void Stage::spawnFloor(const FloorParams& params)
{
    auto paramsCopy = std::make_unique<FloorParams>(params);
    spawn(StageObject(StageObjectType::Floor, std::move(paramsCopy)));
}

StageObject Stage::pop(int time)
{
    StageObject res(StageObjectType::Null);

    // Check if we have more objects to pop
    if (sequenceIndex < sequence.size())
    {
        const auto& obj = sequence[sequenceIndex];
        
        if (time >= obj->start)
        {
            // Move ownership out of the unique_ptr
            res = std::move(*obj);
            
            // Update itemsleft before advancing
            if (res.id == StageObjectType::Ball || res.id == StageObjectType::Hexa) itemsleft--;
            
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
                        static_cast<int>(res.id), res.start, res.x, res.y,
                        ball->size, ball->top, ball->dirX, ball->dirY, ball->ballType);
                }
                else if (auto* floor = res.getParams<FloorParams>())
                {
                    LOG_DEBUG("Pop FLOOR id:%d start:%d x:%d y:%d floorType:%d",
                        static_cast<int>(res.id), res.start, res.x, res.y, floor->floorType);
                }
                else
                {
                    LOG_DEBUG("Pop object id:%d start:%d x:%d y:%d (unknown params type)",
                        static_cast<int>(res.id), res.start, res.x, res.y);
                }
            }
            else
            {
                LOG_DEBUG("Pop object id:%d start:%d x:%d y:%d (no params)",
                    static_cast<int>(res.id), res.start, res.x, res.y);
            }
        }
    }
    
    return res;
}