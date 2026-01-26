#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include <string>
#include <iostream>
#include "sprite2d.h"
#include "sprite.h"
#include "graph.h"
#include "bmfont.h"
#include "logger.h"

// Helper function to convert older RECT usage if any remains
static SDL_Rect toSDLRect(int x, int y, int w, int h) {
    return { x, y, w, h };
}

// Global system font renderer for graph.cpp text rendering
static BMFontRenderer g_systemFontRenderer;

int Graph::init(const char* title, int _mode) {
    mode = _mode;

    if (mode == RENDERMODE_NORMAL)
        return initNormal(title);
    else
        return initEx(title);
}

int Graph::initNormal(const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return 0;
    }

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, RES_X, RES_Y, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        LOG_ERROR("Window could not be created! SDL_Error: %s", SDL_GetError());
        return 0;
    }

    SDL_SetWindowMinimumSize(window, RES_X, RES_Y);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        LOG_ERROR("Renderer could not be created! SDL_Error: %s", SDL_GetError());
        return 0;
    }

    SDL_RenderSetLogicalSize(renderer, RES_X, RES_Y);

    backBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, RES_X, RES_Y);
    if (backBuffer == nullptr) {
        LOG_ERROR("Back buffer could not be created! SDL_Error: %s", SDL_GetError());
        return 0;
    }

    // Initialize system font renderer (no BMFont, uses integrated 5x7 bitmap font)
    g_systemFontRenderer.init(this);

    return 1;
}

int Graph::initEx(const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR("SDL could not initialize! SDL_Error: %s", SDL_GetError());
        return 0;
    }

    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, RES_X, RES_Y, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (window == nullptr) {
        LOG_ERROR("Window could not be created! SDL_Error: %s", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        LOG_ERROR("Renderer could not be created! SDL_Error: %s", SDL_GetError());
        return 0;
    }

    SDL_RenderSetLogicalSize(renderer, RES_X, RES_Y);

    backBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, RES_X, RES_Y);
    if (backBuffer == nullptr) {
        LOG_ERROR("Back buffer could not be created! SDL_Error: %s", SDL_GetError());
        return 0;
    }

    // Initialize system font renderer (no BMFont, uses integrated 5x7 bitmap font)
    g_systemFontRenderer.init(this);

    return 1;
}

void Graph::setFullScreen(bool fs) {
    if (fs)
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    else
        SDL_SetWindowFullscreen(window, 0);
}

void Graph::release() {
    if (backBuffer) {
        SDL_DestroyTexture(backBuffer);
        backBuffer = nullptr;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

void Graph::draw(Sprite* spr, int x, int y) {
    SDL_Rect srcRect = { spr->srcX, spr->srcY, spr->sx, spr->sy };
    SDL_Rect dstRect = { x + spr->xoff, y + spr->yoff, spr->sx, spr->sy };
    SDL_RenderCopy(renderer, spr->bmp, &srcRect, &dstRect);
}

void Graph::draw(Sprite* spr, int x, int y, bool flipHorizontal) {
    SDL_Rect srcRect = { spr->srcX, spr->srcY, spr->sx, spr->sy };
    SDL_Rect dstRect = { x + spr->xoff, y + spr->yoff, spr->sx, spr->sy };

    if (flipHorizontal) {
        SDL_RenderCopyEx(renderer, spr->bmp, &srcRect, &dstRect,
                        0.0,  // angle (no rotation)
                        nullptr,  // center point (use default)
                        SDL_FLIP_HORIZONTAL);
    } else {
        SDL_RenderCopy(renderer, spr->bmp, &srcRect, &dstRect);
    }
}

void Graph::drawScaled(Sprite* spr, int x, int y, int w, int h) {
    SDL_Rect srcRect = { spr->srcX, spr->srcY, spr->sx, spr->sy };
    SDL_Rect dstRect = { x + spr->xoff, y + spr->yoff, w, h };
    SDL_RenderCopy(renderer, spr->bmp, &srcRect, &dstRect);
}

void Graph::draw(SDL_Texture* texture, const SDL_Rect* srcRect, int x, int y) {
    SDL_Rect dstRect = { x, y, srcRect->w, srcRect->h };
    SDL_RenderCopy(renderer, texture, srcRect, &dstRect);
}

// New methods using Sprite2D's internal properties

void Graph::draw(Sprite2D* spr) {
    if (!spr || !spr->isVisible()) return;
    
    Sprite* currentSprite = spr->getCurrentSprite();
    if (!currentSprite || !currentSprite->getBmp()) return;

    // Apply alpha
    if (spr->getAlpha() < 255.0f) {
        SDL_SetTextureAlphaMod(currentSprite->getBmp(), (Uint8)spr->getAlpha());
    }

    // Source rectangle (from sprite sheet)
    SDL_Rect srcRect = { currentSprite->getSrcX(), currentSprite->getSrcY(), currentSprite->getWidth(), currentSprite->getHeight() };

    // Calculate scaled size
    int scaledW = (int)(currentSprite->getWidth() * spr->getScale());
    int scaledH = (int)(currentSprite->getHeight() * spr->getScale());

    // Calculate pivot offset (to keep pivot point fixed during scaling)
    // Pivot is 0.0-1.0 where 0.5, 0.5 = center
    float pivotOffsetX = currentSprite->getWidth() * spr->getPivotX() * (1.0f - spr->getScale());
    float pivotOffsetY = currentSprite->getHeight() * spr->getPivotY() * (1.0f - spr->getScale());

    // Destination rectangle (with sprite offset and pivot adjustment)
    SDL_Rect dstRect = {
        (int)(spr->getX() + currentSprite->getXOff() + pivotOffsetX),
        (int)(spr->getY() + currentSprite->getYOff() + pivotOffsetY),
        scaledW,
        scaledH
    };

    // Pivot point for rotation (within the destination rect)
    SDL_Point center = {
        (int)(scaledW * spr->getPivotX()),
        (int)(scaledH * spr->getPivotY())
    };

    // Flip flags
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (spr->getFlipH() && spr->getFlipV()) {
        flip = (SDL_RendererFlip)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
    } else if (spr->getFlipH()) {
        flip = SDL_FLIP_HORIZONTAL;
    } else if (spr->getFlipV()) {
        flip = SDL_FLIP_VERTICAL;
    }

    // Render with all transformations
    SDL_RenderCopyEx(renderer, currentSprite->getBmp(), &srcRect, &dstRect, spr->getRotation(), &center, flip);

    // Reset alpha if modified
    if (spr->getAlpha() < 255.0f) {
        SDL_SetTextureAlphaMod(currentSprite->getBmp(), 255);
    }
}

void Graph::draw(BmNumFont* font, int num, int x, int y) {
    char cad[16];
    std::snprintf(cad, sizeof(cad), "%d", num);
    draw(font, cad, x, y);
}

void Graph::draw(BmNumFont* font, const std::string& cad, int x, int y) {
    SDL_Rect srcRect;
    int esp = 0;

    for (char c : cad) {
        srcRect = font->getRect(c);
        draw(font->getSprite()->getBmp(), &srcRect, x + esp, y);
        esp += srcRect.w;
    }
}

void Graph::flip() {
    SDL_RenderPresent(renderer);
}

void Graph::text(const char texto[], int x, int y) {

    // Use the system font renderer (integrated 5x7 bitmap font)
    g_systemFontRenderer.text(texto, x, y);
}

void Graph::setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void Graph::rectangle(int a, int b, int c, int d) {
    SDL_Rect rect = { a, b, c - a, d - b };
    SDL_RenderDrawRect(renderer, &rect);
}

void Graph::filledRectangle(int a, int b, int c, int d) {
    SDL_Rect rect = { a, b, c - a, d - b };
    SDL_RenderFillRect(renderer, &rect);
}

void Graph::loadBitmap(Sprite* spr, const char* szBitmap) {
    SDL_Surface* loadedSurface = IMG_Load(szBitmap);
    if (loadedSurface == nullptr) {
        LOG_ERROR("Unable to load image %s! SDL_image Error: %s", szBitmap, IMG_GetError());
        return;
    }

    SDL_SetColorKey(loadedSurface, SDL_TRUE, 0x00FF0000);
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    if (newTexture == nullptr) {
        LOG_ERROR("Unable to create texture from %s! SDL Error: %s", szBitmap, SDL_GetError());
    } else {
        spr->bmp = newTexture;
        spr->sx = loadedSurface->w;
        spr->sy = loadedSurface->h;
    }
    SDL_FreeSurface(loadedSurface);
}

bool Graph::copyBitmap(SDL_Texture*& texture, SDL_Surface* surface, int x, int y, int dx, int dy) {
    if (surface == nullptr) return false;

    SDL_Rect srcRect = { x, y, dx, dy };
    SDL_Rect dstRect = { 0, 0, dx, dy };

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr) return false;

    return SDL_RenderCopy(renderer, texture, &srcRect, &dstRect) == 0;
}

Uint32 Graph::colorMatch(SDL_Surface* surface, Uint32 rgb) {
    // Note: GetRValue etc are Windows macros, using manual shifts or SDL
    return SDL_MapRGB(surface->format, (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
}

bool Graph::setColorKey(SDL_Texture* texture, Uint32 rgb) {
    return true; // SDL2 handles color key during surface->texture conversion
}
