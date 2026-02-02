#pragma once

// Collision side constants - indicate which side of a box was hit
constexpr int SIDE_NONE = 0;
constexpr int SIDE_TOP = 1;
constexpr int SIDE_BOTTOM = 2;
constexpr int SIDE_LEFT = 3;
constexpr int SIDE_RIGHT = 4;

/**
 * @struct CollisionSide
 * @brief Records which side(s) of an entity were hit in a collision
 *
 * Used to determine bounce direction for physics resolution.
 * Both x and y can be set for corner hits.
 */
struct CollisionSide {
    int x = 0;  ///< SIDE_LEFT, SIDE_RIGHT, or 0 (no horizontal collision)
    int y = 0;  ///< SIDE_TOP, SIDE_BOTTOM, or 0 (no vertical collision)

    bool hasHorizontal() const { return x != 0; }
    bool hasVertical() const { return y != 0; }
    bool isCornerHit() const { return x != 0 && y != 0; }
};

/**
 * @struct CollisionBox
 * @brief Axis-aligned bounding box for collision detection
 *
 * Represents a rectangular collision box in top-left coordinate space.
 * Used for AABB (Axis-Aligned Bounding Box) collision detection.
 */
struct CollisionBox {
    int x, y, w, h;

    /**
     * @brief Get the center point of this box
     * @param outX Output x coordinate
     * @param outY Output y coordinate
     */
    void getCenter(int& outX, int& outY) const {
        outX = x + w / 2;
        outY = y + h / 2;
    }

    /**
     * @brief Check if this box has valid dimensions (non-zero area)
     */
    bool isValid() const {
        return w > 0 && h > 0;
    }
};

/**
 * @brief Check if two collision boxes intersect
 * @param a First collision box
 * @param b Second collision box
 * @return true if boxes overlap, false otherwise
 */
inline bool intersects(const CollisionBox& a, const CollisionBox& b)
{
    return a.x < b.x + b.w &&
           a.x + a.w > b.x &&
           a.y < b.y + b.h &&
           a.y + a.h > b.y;
}

/**
 * @brief Check if a point is inside a collision box
 * @param box The collision box
 * @param x Point x coordinate
 * @param y Point y coordinate
 * @return true if point is inside box, false otherwise
 */
inline bool contains(const CollisionBox& box, float x, float y)
{
    return x >= box.x && x < box.x + box.w &&
           y >= box.y && y < box.y + box.h;
}

/**
 * @brief Check if box 'inner' is completely contained within box 'outer'
 * @param outer The containing box
 * @param inner The potentially contained box
 * @return true if inner is fully inside outer, false otherwise
 */
inline bool containsBox(const CollisionBox& outer, const CollisionBox& inner)
{
    return inner.x >= outer.x &&
           inner.y >= outer.y &&
           inner.x + inner.w <= outer.x + outer.w &&
           inner.y + inner.h <= outer.y + outer.h;
}

/**
 * @brief Check if either box is contained within the other
 * @param a First collision box
 * @param b Second collision box
 * @return true if one box is fully inside the other, false otherwise
 */
inline bool hasContainment(const CollisionBox& a, const CollisionBox& b)
{
    return containsBox(a, b) || containsBox(b, a);
}

/**
 * @brief Get the overlap rectangle between two collision boxes
 * @param a First collision box
 * @param b Second collision box
 * @return CollisionBox representing the overlap region (w=0, h=0 if no overlap)
 *
 * The returned box represents the rectangular region where both boxes overlap.
 * If there's no overlap, returns a box with zero width and height.
 */
inline CollisionBox getOverlapRect(const CollisionBox& a, const CollisionBox& b)
{
    // Calculate overlap boundaries
    int left = (a.x > b.x) ? a.x : b.x;
    int right = (a.x + a.w < b.x + b.w) ? a.x + a.w : b.x + b.w;
    int top = (a.y > b.y) ? a.y : b.y;
    int bottom = (a.y + a.h < b.y + b.h) ? a.y + a.h : b.y + b.h;

    // Check if there's actually an overlap
    if (left >= right || top >= bottom)
        return { 0, 0, 0, 0 };

    return { left, top, right - left, bottom - top };
}

/**
 * @brief Get the center point of the overlap between two collision boxes
 * @param a First collision box
 * @param b Second collision box
 * @param outX Output x coordinate of overlap center
 * @param outY Output y coordinate of overlap center
 * @return true if boxes overlap, false otherwise
 *
 * This is useful for determining where a collision "hit" occurred,
 * such as spawning particle effects at the contact point.
 */
inline bool getOverlapCenter(const CollisionBox& a, const CollisionBox& b, int& outX, int& outY)
{
    CollisionBox overlap = getOverlapRect(a, b);
    if (!overlap.isValid())
        return false;

    overlap.getCenter(outX, outY);
    return true;
}
