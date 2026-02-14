#pragma warning(push)
#pragma warning(disable: 4996)

#include "main.h"
#include "configdata.h"
#include "logger.h"
#include <SDL.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

ConfigData::ConfigData()
{
    initPath();
}

ConfigData::~ConfigData()
{
}

void ConfigData::initPath()
{
    char* basePath = SDL_GetPrefPath("Savioret", "Pang");
    if (basePath) {
        configPath = std::string(basePath) + "pang_config.dat";
        SDL_free(basePath);
    } else {
        configPath = "pang_config.dat";
    }
}

void ConfigData::loadDefaults()
{
    globalmode = RENDERMODE_NORMAL;

    gameinf.getKeys(AppData::PLAYER1).setLeft(SDL_SCANCODE_LEFT);
    gameinf.getKeys(AppData::PLAYER1).setRight(SDL_SCANCODE_RIGHT);
    gameinf.getKeys(AppData::PLAYER1).setShoot(SDL_SCANCODE_SPACE);
    gameinf.getKeys(AppData::PLAYER1).setUp(SDL_SCANCODE_UP);
    gameinf.getKeys(AppData::PLAYER1).setDown(SDL_SCANCODE_DOWN);

    gameinf.getKeys(AppData::PLAYER2).setLeft(SDL_SCANCODE_A);
    gameinf.getKeys(AppData::PLAYER2).setRight(SDL_SCANCODE_D);
    gameinf.getKeys(AppData::PLAYER2).setShoot(SDL_SCANCODE_LCTRL);
    gameinf.getKeys(AppData::PLAYER2).setUp(SDL_SCANCODE_W);
    gameinf.getKeys(AppData::PLAYER2).setDown(SDL_SCANCODE_S);
}

bool ConfigData::load()
{
    FILE* fp = fopen(configPath.c_str(), "r");
    if (!fp)
    {
        loadDefaults();
        return false;
    }

    LOG_DEBUG("Loading configuration from %s", configPath.c_str());

    char line[256];
    while (std::fgets(line, sizeof(line), fp))
    {
        if (line[0] == '\n' || line[0] == '#' || line[0] == ';')
            continue;
            
        char key[128], value[128];
        if (sscanf(line, "%127[^=]=%127s", key, value) == 2)
        {
            std::string skey(key);
            if (skey == "P1_Left")
                gameinf.getKeys(AppData::PLAYER1).setLeft((SDL_Scancode)std::atoi(value));
            else if (skey == "P1_Right")
                gameinf.getKeys(AppData::PLAYER1).setRight((SDL_Scancode)std::atoi(value));
            else if (skey == "P1_Shoot")
                gameinf.getKeys(AppData::PLAYER1).setShoot((SDL_Scancode)std::atoi(value));
            else if (skey == "P1_Up")
                gameinf.getKeys(AppData::PLAYER1).setUp((SDL_Scancode)std::atoi(value));
            else if (skey == "P1_Down")
                gameinf.getKeys(AppData::PLAYER1).setDown((SDL_Scancode)std::atoi(value));
            else if (skey == "P2_Left")
                gameinf.getKeys(AppData::PLAYER2).setLeft((SDL_Scancode)std::atoi(value));
            else if (skey == "P2_Right")
                gameinf.getKeys(AppData::PLAYER2).setRight((SDL_Scancode)std::atoi(value));
            else if (skey == "P2_Shoot")
                gameinf.getKeys(AppData::PLAYER2).setShoot((SDL_Scancode)std::atoi(value));
            else if (skey == "P2_Up")
                gameinf.getKeys(AppData::PLAYER2).setUp((SDL_Scancode)std::atoi(value));
            else if (skey == "P2_Down")
                gameinf.getKeys(AppData::PLAYER2).setDown((SDL_Scancode)std::atoi(value));
            else if (skey == "RenderMode")
                globalmode = std::atoi(value);
        }
    }

    std::fclose(fp);
    return true;
}

bool ConfigData::save()
{
    FILE* fp = fopen(configPath.c_str(), "w");
    if ( !fp )
    {
        LOG_ERROR("Failed to open config file for writing: %s", configPath.c_str());
        return false;
    }

    LOG_DEBUG("Saving configuration to %s", configPath.c_str());

    std::fprintf(fp, "# Pang Game Configuration\n");
    std::fprintf(fp, "# Generated automatically - Edit with care\n\n");
    
    std::fprintf(fp, "[Keys]\n");
    std::fprintf(fp, "P1_Left=%d\n", gameinf.getKeys(AppData::PLAYER1).getLeft());
    std::fprintf(fp, "P1_Right=%d\n", gameinf.getKeys(AppData::PLAYER1).getRight());
    std::fprintf(fp, "P1_Shoot=%d\n", gameinf.getKeys(AppData::PLAYER1).getShoot());
    std::fprintf(fp, "P1_Up=%d\n", gameinf.getKeys(AppData::PLAYER1).getUp());
    std::fprintf(fp, "P1_Down=%d\n\n", gameinf.getKeys(AppData::PLAYER1).getDown());

    std::fprintf(fp, "P2_Left=%d\n", gameinf.getKeys(AppData::PLAYER2).getLeft());
    std::fprintf(fp, "P2_Right=%d\n", gameinf.getKeys(AppData::PLAYER2).getRight());
    std::fprintf(fp, "P2_Shoot=%d\n", gameinf.getKeys(AppData::PLAYER2).getShoot());
    std::fprintf(fp, "P2_Up=%d\n", gameinf.getKeys(AppData::PLAYER2).getUp());
    std::fprintf(fp, "P2_Down=%d\n\n", gameinf.getKeys(AppData::PLAYER2).getDown());
    
    std::fprintf(fp, "[Graphics]\n");
    std::fprintf(fp, "RenderMode=%d  # 1=Windowed 1x, 2=Fullscreen, 3=Windowed 2x\n", globalmode);
    
    std::fclose(fp);
    return true;
}

#pragma warning(pop)