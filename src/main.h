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
#include "shoot.h"
#include "floor.h"
#include "minput.h"
#include "appdata.h"
#include "audiomanager.h"

// Object identifiers
constexpr int OBJ_NULL = 0;
constexpr int OBJ_BALL = 1;
constexpr int OBJ_SHOOT = 2;
constexpr int OBJ_FLOOR = 3;
constexpr int OBJ_ITEM = 4;
constexpr int OBJ_PLAYER = 5;

// Animation frames
constexpr int ANIM_LEFT = 0;
constexpr int ANIM_RIGHT = 5;
constexpr int ANIM_SHOOT = 10;
constexpr int ANIM_WIN = 12;
constexpr int ANIM_DEAD = 13;

// Screen boundaries
constexpr int MAX_Y = 415;
constexpr int MIN_Y = 16;
constexpr int MAX_X = 623;
constexpr int MIN_X = 16;

// Player indices
constexpr int PLAYER1 = 0;
constexpr int PLAYER2 = 1;

// Change modes
constexpr int CHANGE_MAIN = 1;
constexpr int CHANGE_SCENE = 2;

// Font types
constexpr int FONT_BIG = 0;
constexpr int FONT_SMALL = 1;
constexpr int FONT_HUGE = 2;

// Collision sides
constexpr int SIDE_TOP = 1;
constexpr int SIDE_BOTTOM = 2;
constexpr int SIDE_LEFT = 3;
constexpr int SIDE_RIGHT = 4;

constexpr int GLOBAL_GAMESPEED = 60;

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
#define CloseMusic() AudioManager::instance().closeAll()