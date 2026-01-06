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

void GRAPH::DrawScaled ( SPRITE* spr, int x, int y, int w, int h ) {
    SDL_Rect srcRect = { 0, 0, spr->sx, spr->sy };
    SDL_Rect dstRect = { x + spr->xoff, y + spr->yoff, w, h };
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
    static const unsigned char font5x7[95][7] = {
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
        {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02}, // \ (fixed)
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

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    int currentX = x;
    int charWidth = 6;
    int charHeight = 8;
    int scale = 2; // Make it more readable

    for (int i = 0; texto[i] != '\0'; i++) {
        unsigned char c = (unsigned char)texto[i];
        if (c < 32 || c > 126) {
           currentX += charWidth * scale;
           continue;
        }
        
        int idx = c - 32;
        for (int row = 0; row < 7; row++) {
            unsigned char bits = font5x7[idx][row];
            for (int col = 0; col < 5; col++) {
                if (bits & (0x80 >> col)) {
                    SDL_Rect pixel = { currentX + col * scale, y + row * scale, scale, scale };
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
        currentX += charWidth * scale;
    }
}
void GRAPH::Rectangle ( int a, int b, int c, int d ) {
    SDL_Rect rect = { a, b, c - a, d - b };
    SDL_RenderDrawRect ( renderer, &rect );
}


/****************************************************************/
/***				OPERACIONES CON BITMAPS					*****/
/****************************************************************/

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