#include <SDL.h>
#include <SDL_image.h>

#include "sprite.h"
#include "graph.h"

/**
 * Sprite initialization
 *
 * xoff and yoff are used to align the sprite within its bounding box.
 * This is useful to avoid the "Saint Vitus dance" (jittery animations)
 * caused by inconsistent sprite dimensions.
 */
void Sprite::init(Graph* gr, const std::string& file, int offx, int offy) {
    graph = gr;
    graph->loadBitmap(this, file.c_str());
    xoff = offx;
    yoff = offy;
}

/**
 * Initialize sprite from a shared texture (for sprite sheets)
 *
 * This doesn't load a new texture - instead it references a region
 * within an existing texture. The sprite does NOT own this texture.
 */
void Sprite::init(SDL_Texture* sharedTexture, int x, int y, int w, int h, int offx, int offy) {
    bmp = sharedTexture;
    srcX = x;
    srcY = y;
    sx = w;
    sy = h;
    xoff = offx;
    yoff = offy;
    graph = nullptr;  // No Graph needed for shared texture
}

void Sprite::release() {
    if (bmp != nullptr) {
        SDL_DestroyTexture(bmp);
        bmp = nullptr;
    }
}