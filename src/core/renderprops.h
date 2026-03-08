#pragma once

// Forward declarations
class Sprite2D;

/**
 * @brief Rendering properties for sprite drawing
 *
 * Contains all properties needed to render a sprite, including position,
 * flipping, rotation, scale, and alpha blending.
 * Kept in a separate header so entities can include it without pulling in
 * the full Graph/SDL headers.
 */
struct RenderProps
{
    int x = 0, y = 0;            ///< Screen position
    bool flipH = false;          ///< Horizontal flip
    bool flipV = false;          ///< Vertical flip
    float rotation = 0.0f;       ///< Rotation angle in degrees
    float scale = 1.0f;          ///< Scale factor
    float alpha = 1.0f;          ///< Alpha transparency (0.0-1.0)
    float pivotX = 0.5f;         ///< Pivot X for rotation (0.0-1.0)
    float pivotY = 0.5f;         ///< Pivot Y for rotation (0.0-1.0)

    /**
     * @brief Default constructor
     */
    RenderProps() = default;

    /**
     * @brief Construct from position
     */
    RenderProps(int x, int y) : x(x), y(y) {}

    /**
     * @brief Construct from Sprite2D properties
     */
    static RenderProps fromSprite2D(Sprite2D* spr);
};
