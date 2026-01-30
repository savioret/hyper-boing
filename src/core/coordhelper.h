#pragma once

#include "sprite.h"

/**
 * Coordinate conversion utilities for bottom-middle pivot system.
 *
 * The game uses two coordinate spaces:
 * - Logical (bottom-middle): Entity position represents the bottom-center point
 * - Render (top-left): SDL2's native positioning where (x,y) is top-left corner
 *
 * Player and Shot entities store positions in logical (bottom-middle) space.
 * Ball, Floor, and other entities remain in render (top-left) space.
 */

// Convert bottom-middle position to top-left render position (basic)
inline int toRenderX(float logicalX, int spriteWidth) {
    return (int)(logicalX - spriteWidth / 2.0f);
}

inline int toRenderY(float logicalY, int spriteHeight) {
    return (int)(logicalY - spriteHeight);
}

// Convert bottom-middle position to top-left render position (sprite-aware)
// Uses sourceSize for correct positioning of trimmed Aseprite sprites.
// drawEx() will add the sprite's xOff/yOff to place the trimmed content correctly.
inline int toRenderX(float logicalX, Sprite* spr) {
    return (int)(logicalX - spr->getSourceWidth() / 2.0f);
}

inline int toRenderY(float logicalY, Sprite* spr) {
    return (int)(logicalY - spr->getSourceHeight());
}

// Convert top-left to bottom-middle (for reading legacy positions)
inline float toLogicalX(int renderX, int spriteWidth) {
    return renderX + spriteWidth / 2.0f;
}

inline float toLogicalY(int renderY, int spriteHeight) {
    return (float)(renderY + spriteHeight);
}
