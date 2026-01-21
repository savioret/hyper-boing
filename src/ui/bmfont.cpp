#include "sprite.h"
#include "graph.h"
#include "bmfont.h"
#include "logger.h"
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
        LOG_ERROR("Could not open BMFont file: %s", fntFilePath);
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

// System font 5x7 bitmap (moved from graph.cpp)
static const unsigned char g_systemFont5x7[95][7] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x20}, // !
    {0x50, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00}, // "
    {0x50, 0x50, 0xF8, 0x50, 0xF8, 0x50, 0x50}, // #
    {0x20, 0x78, 0xA0, 0x70, 0x28, 0xF0, 0x20}, // $
    {0xC8, 0xC8, 0x10, 0x20, 0x40, 0x98, 0x98}, // %
    {0x60, 0x90, 0x90, 0x60, 0x94, 0x90, 0x68}, // &
    {0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
    {0x10, 0x20, 0x40, 0x40, 0x40, 0x20, 0x10}, // (
    {0x40, 0x20, 0x10, 0x10, 0x10, 0x20, 0x40}, // )
    {0x00, 0x50, 0x20, 0xF8, 0x20, 0x50, 0x00}, // *
    {0x00, 0x20, 0x20, 0xF8, 0x20, 0x20, 0x00}, // +
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20}, // ,
    {0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00}, // -
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20}, // .
    {0x00, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00}, // /
    {0x70, 0x88, 0x98, 0xA8, 0xC8, 0x88, 0x70}, // 0
    {0x20, 0x60, 0x20, 0x20, 0x20, 0x20, 0x70}, // 1
    {0x70, 0x88, 0x08, 0x30, 0x40, 0x80, 0xF8}, // 2
    {0xF8, 0x08, 0x10, 0x30, 0x08, 0x88, 0x70}, // 3
    {0x10, 0x30, 0x50, 0x90, 0xF8, 0x10, 0x10}, // 4
    {0xF8, 0x80, 0xF0, 0x08, 0x08, 0x88, 0x70}, // 5
    {0x30, 0x40, 0x80, 0xF0, 0x88, 0x88, 0x70}, // 6
    {0xF8, 0x08, 0x10, 0x20, 0x40, 0x40, 0x40}, // 7
    {0x70, 0x88, 0x88, 0x70, 0x88, 0x88, 0x70}, // 8
    {0x70, 0x88, 0x88, 0x78, 0x08, 0x10, 0x60}, // 9
    {0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x00}, // :
    {0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x40}, // ;
    {0x08, 0x10, 0x20, 0x40, 0x20, 0x10, 0x08}, // <
    {0x00, 0x00, 0xF8, 0x00, 0xF8, 0x00, 0x00}, // =
    {0x40, 0x20, 0x10, 0x08, 0x10, 0x20, 0x40}, // >
    {0x70, 0x88, 0x08, 0x10, 0x20, 0x00, 0x20}, // ?
    {0x70, 0x88, 0x08, 0x68, 0xA8, 0xA0, 0x70}, // @
    {0x20, 0x50, 0x88, 0x88, 0xF8, 0x88, 0x88}, // A
    {0xF0, 0x88, 0x88, 0xF0, 0x88, 0x88, 0xF0}, // B
    {0x70, 0x88, 0x80, 0x80, 0x80, 0x88, 0x70}, // C
    {0xF0, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0}, // D
    {0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0xF8}, // E
    {0xF8, 0x80, 0x80, 0xF0, 0x80, 0x80, 0x80}, // F
    {0x70, 0x88, 0x80, 0x98, 0x88, 0x88, 0x70}, // G
    {0x88, 0x88, 0x88, 0xF8, 0x88, 0x88, 0x88}, // H
    {0x70, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70}, // I
    {0x38, 0x10, 0x10, 0x10, 0x10, 0x90, 0x60}, // J
    {0x88, 0x90, 0xA0, 0xC0, 0xA0, 0x90, 0x88}, // K
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xF8}, // L
    {0x88, 0xD8, 0xA8, 0x88, 0x88, 0x88, 0x88}, // M
    {0x88, 0x88, 0xC8, 0xA8, 0x98, 0x88, 0x88}, // N
    {0x70, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70}, // O
    {0xF0, 0x88, 0x88, 0xF0, 0x80, 0x80, 0x80}, // P
    {0x70, 0x88, 0x88, 0x88, 0xA8, 0x98, 0x70}, // Q
    {0xF0, 0x88, 0x88, 0xF0, 0xA0, 0x90, 0x88}, // R
    {0x70, 0x88, 0x80, 0x70, 0x08, 0x88, 0x70}, // S
    {0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20}, // T
    {0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x70}, // U
    {0x88, 0x88, 0x88, 0x88, 0x88, 0x50, 0x20}, // V
    {0x88, 0x88, 0x88, 0xA8, 0xA8, 0xA8, 0x50}, // W
    {0x88, 0x88, 0x50, 0x20, 0x50, 0x88, 0x88}, // X
    {0x88, 0x88, 0x50, 0x20, 0x20, 0x20, 0x20}, // Y
    {0xF8, 0x08, 0x10, 0x20, 0x40, 0x80, 0xF8}, // Z
    {0x78, 0x40, 0x40, 0x40, 0x40, 0x40, 0x78}, // [
    {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02}, // backslash
    {0x78, 0x08, 0x08, 0x08, 0x08, 0x08, 0x78}, // ]
    {0x20, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00}, // ^
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}, // _
    {0x40, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00}, // `
    {0x00, 0x00, 0x70, 0x08, 0x78, 0x88, 0x78}, // a
    {0x80, 0x80, 0xF0, 0x88, 0x88, 0x88, 0xF0}, // b
    {0x00, 0x00, 0x70, 0x80, 0x80, 0x88, 0x70}, // c
    {0x08, 0x08, 0x78, 0x88, 0x88, 0x88, 0x78}, // d
    {0x00, 0x00, 0x70, 0x88, 0xF8, 0x80, 0x70}, // e
    {0x38, 0x40, 0xF0, 0x40, 0x40, 0x40, 0x40}, // f
    {0x00, 0x78, 0x88, 0x88, 0x78, 0x08, 0x70}, // g
    {0x80, 0x80, 0xF0, 0x88, 0x88, 0x88, 0x88}, // h
    {0x20, 0x00, 0x60, 0x20, 0x20, 0x20, 0x70}, // i
    {0x10, 0x00, 0x30, 0x10, 0x10, 0x90, 0x60}, // j
    {0x80, 0x80, 0x90, 0xA0, 0xC0, 0xA0, 0x90}, // k
    {0x60, 0x20, 0x20, 0x20, 0x20, 0x20, 0x70}, // l
    {0x00, 0x00, 0xEC, 0x92, 0x92, 0x92, 0x92}, // m
    {0x00, 0x00, 0xF0, 0x88, 0x88, 0x88, 0x88}, // n
    {0x00, 0x00, 0x70, 0x88, 0x88, 0x88, 0x70}, // o
    {0x00, 0x00, 0xF0, 0x88, 0x88, 0xF0, 0x80}, // p
    {0x00, 0x00, 0x78, 0x88, 0x88, 0x78, 0x08}, // q
    {0x00, 0x00, 0xB0, 0x48, 0x40, 0x40, 0x40}, // r
    {0x00, 0x00, 0x78, 0x80, 0x70, 0x08, 0xF0}, // s
    {0x40, 0x40, 0xF0, 0x40, 0x40, 0x40, 0x30}, // t
    {0x00, 0x00, 0x88, 0x88, 0x88, 0x88, 0x78}, // u
    {0x00, 0x00, 0x88, 0x88, 0x88, 0x50, 0x20}, // v
    {0x00, 0x00, 0x88, 0x88, 0xA8, 0xA8, 0x50}, // w
    {0x00, 0x00, 0x88, 0x50, 0x20, 0x50, 0x88}, // x
    {0x00, 0x00, 0x88, 0x88, 0x78, 0x08, 0x70}, // y
    {0x00, 0x00, 0xF8, 0x10, 0x20, 0x40, 0xF8}  // z
};

BMFontRenderer::BMFontRenderer()
    : fontLoader(nullptr), fontTexture(nullptr), graph(nullptr),
      colorR(255), colorG(255), colorB(255), colorA(255), scale(1.0f)
{
}

bool BMFontRenderer::loadFont(Graph* gr, const char* fntPath, const char* texturePath)
{
    if (!gr || !fntPath)
    {
        LOG_ERROR("Invalid parameters for BMFontRenderer::loadFont");
        return false;
    }
    
    graph = gr;
    
    // Load .fnt file
    fontLoader = std::make_unique<BMFontLoader>();
    if (!fontLoader->load(fntPath))
    {
        LOG_ERROR("Failed to load font file: %s", fntPath);
        fontLoader.reset();
        return false;
    }
    
    // Determine texture path
    std::string textureFile;
    
    if (texturePath && texturePath[0] != '\0')
    {
        // Use explicitly provided texture path
        textureFile = texturePath;
    }
    else
    {
        // Use texture path from .fnt file (always present in BMFont format)
        const std::string& fntTexture = fontLoader->getFontTexture();
        
        if (fntTexture.empty())
        {
            LOG_ERROR("No texture specified in font file: %s", fntPath);
            fontLoader.reset();
            return false;
        }
        
        // Make texture path relative to .fnt file location
        std::string fntDir = fntPath;
        size_t lastSlash = fntDir.find_last_of("/\\");
        if (lastSlash != std::string::npos)
        {
            fntDir = fntDir.substr(0, lastSlash + 1);
            textureFile = fntDir + fntTexture;
        }
        else
        {
            textureFile = fntTexture;
        }
    }
    
    // Load texture
    fontTexture = std::make_unique<Sprite>();
    fontTexture->init(graph, textureFile.c_str(), 0, 0);
    
    if (!fontTexture->getBmp())
    {
        LOG_ERROR("Failed to load font texture: %s", textureFile.c_str());
        fontLoader.reset();
        fontTexture.reset();
        return false;
    }
    
    // Initialize rendering state
    colorR = 255;
    colorG = 255;
    colorB = 255;
    colorA = 255;
    scale = 1.0f;
    
    LOG_SUCCESS("Font loaded successfully: %s -> %s", fntPath, textureFile.c_str());
    
    return true;
}

void BMFontRenderer::init(Graph* gr, BMFontLoader* bmFont, Sprite* texture)
{
    graph = gr;
    
    // Legacy API: external management (non-owning pointers)
    // Note: This is deprecated, use loadFont() for simplified usage
    fontLoader.reset();  // Release any internally managed loader
    fontTexture.reset(); // Release any internally managed texture
    
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

void BMFontRenderer::renderSystemFont(const char* texto, int x, int y)
{
    if (!graph) return;
    
    SDL_SetRenderDrawColor(graph->getRenderer(), colorR, colorG, colorB, colorA);
    
    int currentX = x;
    int charWidth = 6;  // 5 pixels + 1 pixel spacing
    int pixelScale = (int)scale;
    if (pixelScale < 1) pixelScale = 1;
    
    for (int i = 0; texto[i] != '\0'; i++)
    {
        unsigned char c = (unsigned char)texto[i];
        if (c < 32 || c > 126)
        {
            currentX += charWidth * pixelScale;
            continue;
        }
        
        int idx = c - 32;
        for (int row = 0; row < 7; row++)
        {
            unsigned char bits = g_systemFont5x7[idx][row];
            for (int col = 0; col < 5; col++)
            {
                if (bits & (0x80 >> col))
                {
                    SDL_Rect pixel = { 
                        currentX + col * pixelScale, 
                        y + row * pixelScale, 
                        pixelScale, 
                        pixelScale 
                    };
                    SDL_RenderFillRect(graph->getRenderer(), &pixel);
                }
            }
        }
        currentX += charWidth * pixelScale;
    }
}

int BMFontRenderer::getSystemFontTextWidth(const char* texto) const
{
    int charWidth = 6;  // 5 pixels + 1 pixel spacing
    int pixelScale = (int)scale;
    if (pixelScale < 1) pixelScale = 1;
    
    int length = 0;
    for (int i = 0; texto[i] != '\0'; i++)
        length++;
    
    return length * charWidth * pixelScale;
}

void BMFontRenderer::text(const char* texto, int x, int y)
{
    // Use system font if no BMFont is loaded
    if (!fontLoader || !fontTexture)
    {
        renderSystemFont(texto, x, y);
        return;
    }
    
    // Use BMFont rendering
    SDL_SetTextureColorMod(fontTexture->getBmp(), colorR, colorG, colorB);
    SDL_SetTextureAlphaMod(fontTexture->getBmp(), colorA);

    int currentX = x;
    
    for (int i = 0; texto[i] != '\0'; i++)
    {
        int charId = (int)((unsigned char)texto[i]);
        const BMFontChar* ch = fontLoader->getChar(charId);

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
            currentX += (int)(fontLoader->getLineHeight() * 0.5f * scale);
        }
    }

    SDL_SetTextureColorMod(fontTexture->getBmp(), 255, 255, 255);
    SDL_SetTextureAlphaMod(fontTexture->getBmp(), 255);
}

int BMFontRenderer::getTextWidth(const char* texto) const
{
    // Use system font if no BMFont is loaded
    if (!fontLoader)
    {
        return getSystemFontTextWidth(texto);
    }
    
    // Use BMFont width calculation
    int width = 0;
    for (int i = 0; texto[i] != '\0'; i++)
    {
        int charId = (int)((unsigned char)texto[i]);
        const BMFontChar* ch = fontLoader->getChar(charId);

        if (ch != nullptr)
            width += (int)(ch->xadvance * scale);
        else
            width += (int)(fontLoader->getLineHeight() * 0.5f * scale);
    }
    return width;
}

int BMFontRenderer::getTextHeight() const
{
    if (!fontLoader)
    {
        // System font height: 7 pixels + 1 pixel spacing = 8
        int pixelScale = (int)scale;
        if (pixelScale < 1) pixelScale = 1;
        return 8 * pixelScale;
    }
    
    return (int)(fontLoader->getLineHeight() * scale);
}

void BMFontRenderer::release()
{
    fontLoader.reset();
    fontTexture.reset();
    graph = nullptr;
}