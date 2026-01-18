#include "pang.h"
#include "configdata.h"
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
    
    gameinf.getKeys()[0].left = SDL_SCANCODE_LEFT;
    gameinf.getKeys()[0].right = SDL_SCANCODE_RIGHT;
    gameinf.getKeys()[0].shoot = SDL_SCANCODE_SPACE;
    
    gameinf.getKeys()[1].left = SDL_SCANCODE_A;
    gameinf.getKeys()[1].right = SDL_SCANCODE_D;
    gameinf.getKeys()[1].shoot = SDL_SCANCODE_LCTRL;
}

bool ConfigData::load()
{
    FILE* fp = nullptr;
    if (fopen_s(&fp, configPath.c_str(), "r") != 0)
    {
        loadDefaults();
        return false;
    }

    char line[256];
    while (std::fgets(line, sizeof(line), fp))
    {
        if (line[0] == '\n' || line[0] == '#' || line[0] == ';')
            continue;
            
        char key[128], value[128];
        if (sscanf_s(line, "%127[^=]=%127s", key, (unsigned)sizeof(key), value, (unsigned)sizeof(value)) == 2)
        {
            std::string skey(key);
            if (skey == "P1_Left")
                gameinf.getKeys()[0].left = (SDL_Scancode)std::atoi(value);
            else if (skey == "P1_Right")
                gameinf.getKeys()[0].right = (SDL_Scancode)std::atoi(value);
            else if (skey == "P1_Shoot")
                gameinf.getKeys()[0].shoot = (SDL_Scancode)std::atoi(value);
            else if (skey == "P2_Left")
                gameinf.getKeys()[1].left = (SDL_Scancode)std::atoi(value);
            else if (skey == "P2_Right")
                gameinf.getKeys()[1].right = (SDL_Scancode)std::atoi(value);
            else if (skey == "P2_Shoot")
                gameinf.getKeys()[1].shoot = (SDL_Scancode)std::atoi(value);
            else if (skey == "RenderMode")
                globalmode = std::atoi(value);
        }
    }

    std::fclose(fp);
    return true;
}

bool ConfigData::save()
{
    FILE* fp = nullptr;
    if (fopen_s(&fp, configPath.c_str(), "w") != 0)
        return false;

    std::fprintf(fp, "# Pang Game Configuration\n");
    std::fprintf(fp, "# Generated automatically - Edit with care\n\n");
    
    std::fprintf(fp, "[Keys]\n");
    std::fprintf(fp, "P1_Left=%d\n", gameinf.getKeys()[0].left);
    std::fprintf(fp, "P1_Right=%d\n", gameinf.getKeys()[0].right);
    std::fprintf(fp, "P1_Shoot=%d\n\n", gameinf.getKeys()[0].shoot);
    
    std::fprintf(fp, "P2_Left=%d\n", gameinf.getKeys()[1].left);
    std::fprintf(fp, "P2_Right=%d\n", gameinf.getKeys()[1].right);
    std::fprintf(fp, "P2_Shoot=%d\n\n", gameinf.getKeys()[1].shoot);
    
    std::fprintf(fp, "[Graphics]\n");
    std::fprintf(fp, "RenderMode=%d  # 1=Windowed, 2=Fullscreen\n", globalmode);
    
    std::fclose(fp);
    return true;
}
