#include <stdio.h>
#include <iostream>
#include "pang.h"
//#include "wingdi.h"

SDL_Rect toSDLRect ( const RECT& rect ) {
    SDL_Rect sdlRect;
    sdlRect.x = rect.left;
    sdlRect.y = rect.top;
    sdlRect.w = rect.right - rect.left;
    sdlRect.h = rect.bottom - rect.top;
    return sdlRect;
}

// Main Initialization Function
int GRAPH::Init ( const char* title, int _mode ) {
    mode = _mode;

    if ( mode == RENDERMODE_NORMAL )
        return InitNormal ( title );
    else
        return InitEx ( title );
}

// Normal Windowed Mode Initialization
int GRAPH::InitNormal ( const char* title ) {
    if ( SDL_Init ( SDL_INIT_VIDEO ) < 0 ) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError () << std::endl;
        return 0;
    }

    window = SDL_CreateWindow ( title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, RES_X, RES_Y, SDL_WINDOW_SHOWN );
    if ( window == nullptr ) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError () << std::endl;
        return 0;
    }

    renderer = SDL_CreateRenderer ( window, -1, SDL_RENDERER_ACCELERATED );
    if ( renderer == nullptr ) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError () << std::endl;
        return 0;
    }

    backBuffer = SDL_CreateTexture ( renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, RES_X, RES_Y );
    if ( backBuffer == nullptr ) {
        std::cerr << "Back buffer could not be created! SDL_Error: " << SDL_GetError () << std::endl;
        return 0;
    }

    return 1;
}

// Fullscreen Exclusive Mode Initialization
int GRAPH::InitEx ( const char* title ) {
    if ( SDL_Init ( SDL_INIT_VIDEO ) < 0 ) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError () << std::endl;
        return 0;
    }

    window = SDL_CreateWindow ( title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, RES_X, RES_Y, SDL_WINDOW_FULLSCREEN );
    if ( window == nullptr ) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError () << std::endl;
        return 0;
    }

    renderer = SDL_CreateRenderer ( window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    if ( renderer == nullptr ) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError () << std::endl;
        return 0;
    }

    backBuffer = SDL_CreateTexture ( renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, RES_X, RES_Y );
    if ( backBuffer == nullptr ) {
        std::cerr << "Back buffer could not be created! SDL_Error: " << SDL_GetError () << std::endl;
        return 0;
    }

    return 1;
}

// Release resources
void GRAPH::Release () {
    if ( backBuffer ) {
        SDL_DestroyTexture ( backBuffer );
        backBuffer = nullptr;
    }
    if ( renderer ) {
        SDL_DestroyRenderer ( renderer );
        renderer = nullptr;
    }
    if ( window ) {
        SDL_DestroyWindow ( window );
        window = nullptr;
    }
    SDL_Quit ();
}


void GRAPH::Draw ( SPRITE* spr, int x, int y ) {
    SDL_Rect srcRect = { 0, 0, spr->sx, spr->sy };
    SDL_Rect dstRect = { x + spr->xoff, y + spr->yoff, spr->sx, spr->sy };
    SDL_RenderCopy ( renderer, spr->bmp, &srcRect, &dstRect );
}

void GRAPH::Draw ( SDL_Texture* texture, RECT* rect, int x, int y ) {
    SDL_Rect srcRect = toSDLRect ( *rect );
    SDL_Rect dstRect = { x, y, srcRect.w, srcRect.h };

    // SDL_QueryTexture ( texture, NULL, NULL, &dstRect.w, &dstRect.h ); // ?????
    SDL_RenderCopy ( renderer, texture, &srcRect, &dstRect );
}


void GRAPH::DrawClipped ( SDL_Texture* texture, RECT* rect, int x, int y ) {
    SDL_Rect srcRect = toSDLRect ( *rect );
    int sx = srcRect.w;
    int sy = srcRect.h;

    if ( x < 0 ) {
        srcRect.x = -x;
        srcRect.w = sx + x;
        x = 0;
    }
    if ( x + sx > 640 ) {
        srcRect.w = 640 - x;
    }
    if ( y < 0 ) {
        srcRect.y = -y;
        srcRect.h = sy + y;
        y = 0;
    }
    if ( y + sy > 480 ) {
        srcRect.h = 480 - y;
    }

    SDL_Rect dstRect = { x, y, srcRect.w, srcRect.h };
    SDL_RenderCopy ( renderer, texture, &srcRect, &dstRect );
}

void GRAPH::DrawClipped ( SPRITE* spr, int x, int y ) {
    SDL_Rect srcRect = { 0, 0, spr->sx, spr->sy };

    if ( x < 0 ) {
        srcRect.x = -x;
        srcRect.w = spr->sx + x;
        x = 0;
    }
    if ( x + spr->sx > 640 ) {
        srcRect.w = 640 - x;
    }
    if ( y < 0 ) {
        srcRect.y = -y;
        srcRect.h = spr->sy + y;
        y = 0;
    }
    if ( y + spr->sy > 480 ) {
        srcRect.h = 480 - y;
    }

    SDL_Rect dstRect = { x + spr->xoff, y + spr->yoff, srcRect.w, srcRect.h };
    SDL_RenderCopy ( renderer, spr->bmp, &srcRect, &dstRect );
}

void GRAPH::Draw ( BMNUMFONT* font, int num, int x, int y ) {
    RECT srcRect;
    char cad[16];
    int n, i;
    int esp = 0;

    sprintf ( cad, "%d", num );
    n = strlen ( cad );

    for ( i = 0; i < n; i++ ) {
        srcRect = font->GetRect ( cad[i] );
        //SDL_Rect sdlRect = toSDLRect ( srcRect );
        Draw ( font->spr->bmp, &srcRect, x + esp, y );
        esp += srcRect.right - srcRect.left;
    }
}

void GRAPH::Draw ( BMNUMFONT* font, char cad[], int x, int y ) {
    RECT srcRect;
    int n, i;
    int esp = 0;

    n = strlen ( cad );

    for ( i = 0; i < n; i++ ) {
        srcRect = font->GetRect ( cad[i] );
        Draw ( font->spr->bmp, &srcRect, x + esp, y );
        esp += srcRect.right - srcRect.left;
    }
}

void GRAPH::DrawClipped ( BMNUMFONT* font, char cad[], int x, int y ) {
    RECT srcRect;
    int n, i;
    int esp = 0;

    n = strlen ( cad );

    for ( i = 0; i < n; i++ ) {
        srcRect = font->GetRect ( cad[i] );
        DrawClipped ( font->spr->bmp, &srcRect, x + esp, y );
        esp += srcRect.right - srcRect.left;
    }
}

void GRAPH::Flip () {
    SDL_RenderPresent ( renderer );
}

void GRAPH::Text ( const char texto[], int x, int y ) {
    // Implementación básica sin SDL_TTF
    // Dibujamos rectángulos pequeños para simular caracteres
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    int charWidth = 8;
    int charHeight = 12;
    int currentX = x;
    
    for (int i = 0; texto[i] != '\0'; i++) {
        char c = texto[i];
        
        // Espacio
        if (c == ' ') {
            currentX += charWidth;
            continue;
        }
        
        // Dibujar un rectángulo simple para cada carácter
        SDL_Rect charRect = {currentX + 1, y + 1, charWidth - 2, charHeight - 2};
        SDL_RenderFillRect(renderer, &charRect);
        
        // Dibujar algunos "detalles" para diferenciar caracteres
        if (c >= 'A' && c <= 'Z') {
            // Letra mayúscula - línea superior
            SDL_SetRenderDrawColor(renderer, 200, 200, 255, 255);
            SDL_RenderDrawLine(renderer, currentX + 2, y + 3, currentX + charWidth - 2, y + 3);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
        else if (c >= 'a' && c <= 'z') {
            // Letra minúscula - línea media
            SDL_SetRenderDrawColor(renderer, 255, 255, 200, 255);
            SDL_RenderDrawLine(renderer, currentX + 2, y + charHeight/2, currentX + charWidth - 2, y + charHeight/2);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
        else if (c >= '0' && c <= '9') {
            // Número - más pequeño
            charRect.w = charWidth - 3;
            charRect.h = charHeight - 3;
            SDL_SetRenderDrawColor(renderer, 255, 200, 200, 255);
            SDL_RenderFillRect(renderer, &charRect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
        
        currentX += charWidth;
    }
}
void GRAPH::Rectangle ( int a, int b, int c, int d ) {
    SDL_Rect rect = { a, b, c - a, d - b };
    SDL_RenderDrawRect ( renderer, &rect );
}


/****************************************************************/
/***				OPERACIONES CON BITMAPS					*****/
/****************************************************************/
/*void GRAPH::LoadBitmap (SPRITE* spr, const char* szBitmap) {

    // TODO: IMG_LoadTexture instead
    SDL_Surface* surface = SDL_LoadTexture ( szBitmap );
    if ( surface == nullptr ) {
        std::cerr << "Failed to load bitmap: " << SDL_GetError () << std::endl;
        return;
    }

    spr->sx = surface->w;
    spr->sy = surface->h;
    spr->bmp = SDL_CreateTextureFromSurface ( renderer, surface );
    SDL_FreeSurface ( surface );

    if ( spr->bmp == nullptr ) {
        std::cerr << "Failed to create texture from surface: " << SDL_GetError () << std::endl;
        return;
    }
}*/

void GRAPH::LoadBitmap ( SPRITE* spr, const char* szBitmap )
{
    //The final texture
    SDL_Texture* newTexture = NULL;

    //Load image at specified path
    SDL_Surface* loadedSurface = IMG_Load ( szBitmap );
    if ( loadedSurface == NULL )
    {
        printf ( "Unable to load image %s! SDL_image Error: %s\n", szBitmap, IMG_GetError () );
    }
    else
    {
        // TODO: this mask should be passed as param
        SDL_SetColorKey ( loadedSurface, SDL_TRUE, 0x00FF0000 );

        //Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface ( renderer, loadedSurface );
        if ( newTexture == NULL )
        {
            printf ( "Unable to create texture from %s! SDL Error: %s\n", szBitmap, SDL_GetError () );
        }

        spr->bmp = newTexture;
        spr->sx = loadedSurface->w;
        spr->sy = loadedSurface->h;

        //Get rid of old loaded surface
        SDL_FreeSurface ( loadedSurface );
    }

}


HRESULT GRAPH::CopyBitmap ( SDL_Texture* texture, SDL_Surface* surface, int x, int y, int dx, int dy ) {
    if ( texture == nullptr || surface == nullptr ) {
        return E_FAIL;
    }

    SDL_Rect srcRect = { x, y, dx, dy };
    SDL_Rect dstRect = { 0, 0, dx, dy }; // Destination rect starts at the top-left corner

    // Create texture from surface
    texture = SDL_CreateTextureFromSurface ( renderer, surface );
    if ( texture == nullptr ) {
        std::cerr << "Failed to create texture from surface: " << SDL_GetError () << std::endl;
        return E_FAIL;
    }

    // Copy the surface onto the texture
    if ( SDL_RenderCopy ( renderer, texture, &srcRect, &dstRect ) != 0 ) {
        std::cerr << "Failed to copy surface to texture: " << SDL_GetError () << std::endl;
        return E_FAIL;
    }

    return S_OK;
}


Uint32 GRAPH::ColorMatch ( SDL_Surface* surface, Uint32 rgb ) {
    Uint32 result = SDL_MapRGB ( surface->format, GetRValue ( rgb ), GetGValue ( rgb ), GetBValue ( rgb ) );
    return result;
}

HRESULT GRAPH::SetColorKey ( SDL_Texture* texture, Uint32 rgb ) {

    // Set the color key
    //SDL_SetColorKey ( texture, SDL_TRUE, rgb );

    return S_OK;
}



//================================================
void SPRITE::Init(GRAPH *gr, char file[], int offx, int offy)
{
	graph = gr;
	graph->LoadBitmap( this, file);

	xoff = offx;
	yoff = offy;
}

void SPRITE::Release ( )
{
    if ( bmp != NULL ) {
        SDL_DestroyTexture ( bmp );
        bmp = NULL;
    }
}