#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <vector>

#include "app.h"
#include "graph.h"
#include "bmfont.h"
#include "scene.h"
#include "menu.h"
#include "select.h"
#include "stageclear.h"
#include "configscreen.h"
#include "ball.h"
#include "player.h"
#include "item.h"
#include "floor.h"
#include "minput.h"
#include "appdata.h"
#include "audiomanager.h"

// Legacy compatibility macros - for gradual migration
#define gameinf AppData::instance()
#define quit AppData::instance().quit
#define goback AppData::instance().goBack
#define globalmode AppData::instance().renderMode

// Audio compatibility macros - map old functions to AudioManager
#define OpenMusic(file) AudioManager::instance().openMusic(file)
#define PlayMusic() AudioManager::instance().play()
#define StopMusic() AudioManager::instance().stop()
#define ContinueMusic() AudioManager::instance().resume()
#define CloseMusic() AudioManager::instance().closeMusic()