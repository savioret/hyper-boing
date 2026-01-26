#include "gameevent.h"
#include <SDL.h>

GameEventData::GameEventData(GameEventType t) : type(t)
{
    timestamp = SDL_GetTicks();
}