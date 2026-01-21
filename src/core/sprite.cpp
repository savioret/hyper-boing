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

void Sprite::release() {
    if (bmp != nullptr) {
        SDL_DestroyTexture(bmp);
        bmp = nullptr;
    }
}