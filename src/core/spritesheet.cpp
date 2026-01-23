#include "spritesheet.h"
#include "graph.h"
#include <SDL_image.h>

SpriteSheet::SpriteSheet()
    : texture(nullptr)
{
}

SpriteSheet::~SpriteSheet()
{
    release();
}

bool SpriteSheet::init(Graph* gr, const std::string& file)
{
    // Release any existing texture
    release();

    // Load the surface from file
    SDL_Surface* surface = IMG_Load(file.c_str());
    if (!surface)
    {
        return false;
    }

    // Create texture from surface
    texture = SDL_CreateTextureFromSurface(gr->getRenderer(), surface);
    SDL_FreeSurface(surface);

    if (!texture)
    {
        return false;
    }

    // Clear any existing frames
    frames.clear();

    return true;
}

void SpriteSheet::addFrame(int x, int y, int w, int h, int xoff, int yoff)
{
    Sprite frame;
    frame.init(texture, x, y, w, h, xoff, yoff);
    frames.push_back(frame);
}

void SpriteSheet::release()
{
    if (texture)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    frames.clear();
}

Sprite* SpriteSheet::getFrame(int index)
{
    if (index >= 0 && index < static_cast<int>(frames.size()))
    {
        return &frames[index];
    }
    return nullptr;
}
