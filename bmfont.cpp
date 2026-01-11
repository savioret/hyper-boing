#include "sprite.h"
#include "graph.h"
#include "bmfont.h"
#include <fstream>
#include <sstream>
#include <cstring>

void BmNumFont::init(Sprite* s)
{
    sprite = s;
    numChars = 10;
}

void BmNumFont::setValue(int number, int xOff)
{
    if (number >= 0 && number <= 9)
        offsets[number] = xOff;
}

void BmNumFont::setValues(const int* xOff)
{
    for (int i = 0; i < numChars; i++)
        offsets[i] = xOff[i];
}

SDL_Rect BmNumFont::getRect(char numChar) const
{
    SDL_Rect rcchr = { 0, 0, 0, 0 };
    int code = numChar - '0';

    if (code >= 0 && code <= 9)
    {
        rcchr.y = 0;
        rcchr.h = sprite->getHeight();
        rcchr.x = offsets[code];
        if (code == numChars - 1) 
            rcchr.w = sprite->getWidth() - offsets[code];
        else 
            rcchr.w = offsets[code + 1] - offsets[code];
    }

    return rcchr;
}

/********************************************************
  BMFontLoader Implementation
********************************************************/

BMFontLoader::BMFontLoader()
    : lineHeight(0), base(0), scaleW(0), scaleH(0), pages(0)
{
}

bool BMFontLoader::load(const char* fntFilePath)
{
    std::ifstream file(fntFilePath);
    if (!file.is_open())
    {
        printf("Error: Could not open BMFont file: %s\n", fntFilePath);
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        parseLine(line);
    }

    file.close();
    return true;
}

bool BMFontLoader::parseLine(const std::string& line)
{
    if (line.empty()) return true;

    std::istringstream iss(line);
    std::string type;
    iss >> type;

    if (type == "common")
    {
        std::string token;
        while (iss >> token)
        {
            size_t pos = token.find('=');
            if (pos != std::string::npos)
            {
                std::string key = token.substr(0, pos);
                int value = std::stoi(token.substr(pos + 1));

                if (key == "lineHeight") lineHeight = value;
                else if (key == "base") base = value;
                else if (key == "scaleW") scaleW = value;
                else if (key == "scaleH") scaleH = value;
                else if (key == "pages") pages = value;
            }
        }
    }
    else if (type == "page")
    {
        std::string token;
        while (iss >> token)
        {
            size_t pos = token.find('=');
            if (pos != std::string::npos)
            {
                std::string key = token.substr(0, pos);
                std::string value = token.substr(pos + 1);

                if (key == "file")
                {
                    if (value.front() == '"') value = value.substr(1);
                    if (value.back() == '"') value.pop_back();
                    fontTexture = value;
                }
            }
        }
    }
    else if (type == "char")
    {
        BMFontChar character = {};
        std::string token;
        while (iss >> token)
        {
            size_t pos = token.find('=');
            if (pos != std::string::npos)
            {
                std::string key = token.substr(0, pos);
                int value = std::stoi(token.substr(pos + 1));

                if (key == "id") character.id = value;
                else if (key == "x") character.x = value;
                else if (key == "y") character.y = value;
                else if (key == "width") character.width = value;
                else if (key == "height") character.height = value;
                else if (key == "xoffset") character.xoffset = value;
                else if (key == "yoffset") character.yoffset = value;
                else if (key == "xadvance") character.xadvance = value;
                else if (key == "page") character.page = value;
            }
        }
        characters[character.id] = character;
    }

    return true;
}

const BMFontChar* BMFontLoader::getChar(int charId) const
{
    auto it = characters.find(charId);
    if (it != characters.end())
    {
        return &it->second;
    }
    return nullptr;
}

/********************************************************
  BMFontRenderer Implementation
********************************************************/

BMFontRenderer::BMFontRenderer()
    : font(nullptr), fontTexture(nullptr), graph(nullptr),
      colorR(255), colorG(255), colorB(255), colorA(255), scale(1.0f)
{
}

void BMFontRenderer::init(Graph* gr, BMFontLoader* bmFont, Sprite* texture)
{
    graph = gr;
    font = bmFont;
    fontTexture = texture;
    colorR = 255;
    colorG = 255;
    colorB = 255;
    colorA = 255;
    scale = 1.0f;
}

void BMFontRenderer::setColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    colorR = r;
    colorG = g;
    colorB = b;
    colorA = a;
}

void BMFontRenderer::setScale(float s)
{
    scale = s;
}

void BMFontRenderer::text(const char* texto, int x, int y)
{
    if (!font || !fontTexture || !graph) return;

    SDL_SetTextureColorMod(fontTexture->getBmp(), colorR, colorG, colorB);
    SDL_SetTextureAlphaMod(fontTexture->getBmp(), colorA);

    int currentX = x;
    
    for (int i = 0; texto[i] != '\0'; i++)
    {
        int charId = (int)((unsigned char)texto[i]);
        const BMFontChar* ch = font->getChar(charId);

        if (ch != nullptr && ch->width > 0 && ch->height > 0)
        {
            SDL_Rect srcRect = { ch->x, ch->y, ch->width, ch->height };
            SDL_Rect dstRect = { 
                currentX + static_cast<int>(ch->xoffset * scale), 
                y + static_cast<int>(ch->yoffset * scale), 
                static_cast<int>(ch->width * scale), 
                static_cast<int>(ch->height * scale) 
            };

            SDL_RenderCopy(graph->getRenderer(), fontTexture->getBmp(), &srcRect, &dstRect);
            currentX += (int)(ch->xadvance * scale);
        }
        else
        {
            currentX += (int)(font->getLineHeight() * 0.5f * scale);
        }
    }

    SDL_SetTextureColorMod(fontTexture->getBmp(), 255, 255, 255);
    SDL_SetTextureAlphaMod(fontTexture->getBmp(), 255);
}

int BMFontRenderer::getTextWidth(const char* texto) const
{
    if (!font) return 0;

    int width = 0;
    for (int i = 0; texto[i] != '\0'; i++)
    {
        int charId = (int)((unsigned char)texto[i]);
        const BMFontChar* ch = font->getChar(charId);

        if (ch != nullptr)
            width += (int)(ch->xadvance * scale);
        else
            width += (int)(font->getLineHeight() * 0.5f * scale);
    }
    return width;
}

int BMFontRenderer::getTextHeight() const
{
    if (!font) return 0;
    return (int)(font->getLineHeight() * scale);
}

void BMFontRenderer::release()
{
    font = nullptr;
    fontTexture = nullptr;
    graph = nullptr;
}