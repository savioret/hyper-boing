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

/**
 * @brief Calculate horizontal overlap between two ranges
 * @param left1 Left edge of first range
 * @param right1 Right edge of first range
 * @param left2 Left edge of second range
 * @param right2 Right edge of second range
 * @return Overlap width in pixels (0 if no overlap)
 */
inline int calculateHorizontalOverlap(int left1, int right1, int left2, int right2)
{
    int overlapLeft = (left1 > left2) ? left1 : left2;
    int overlapRight = (right1 < right2) ? right1 : right2;
    int overlap = overlapRight - overlapLeft;
    return (overlap > 0) ? overlap : 0;
}

/**
 * @brief Check if two collision boxes have 50% horizontal overlap
 *
 * Uses the "50% rule": the narrower box must be at least 50% inside
 * the wider box horizontally. Used for ladder entry detection.
 *
 * @param a First collision box
 * @param b Second collision box
 * @return true if 50% overlap criteria is met
 */
inline bool has50PercentOverlap(const CollisionBox& a, const CollisionBox& b)
{
    int overlapWidth = calculateHorizontalOverlap(
        a.x, a.x + a.w,
        b.x, b.x + b.w
    );
    int narrower = (a.w < b.w) ? a.w : b.w;
    return overlapWidth >= narrower / 2;
}

/**
 * @brief Check if a circle intersects a collision box
 *
 * Uses closest-point-on-rectangle algorithm for accurate circle-rectangle
 * collision detection. More accurate than AABB for circular objects like balls.
 *
 * @param cx Circle center x coordinate
 * @param cy Circle center y coordinate
 * @param radius Circle radius
 * @param box The collision box to test against
 * @return true if circle overlaps box, false otherwise
 */
inline bool circleIntersectsBox(int cx, int cy, int radius, const CollisionBox& box)
{
    // Find closest point on rectangle to circle center
    int closestX = (cx < box.x) ? box.x :
                   (cx > box.x + box.w) ? box.x + box.w : cx;
    int closestY = (cy < box.y) ? box.y :
                   (cy > box.y + box.h) ? box.y + box.h : cy;

    // Calculate squared distance from circle center to closest point
    int dx = cx - closestX;
    int dy = cy - closestY;
    int distSq = dx * dx + dy * dy;

    return distSq <= radius * radius;
}

/**
 * @brief Get collision side when circle hits rectangle
 *
 * Determines which side(s) of the rectangle the circle is colliding with
 * based on penetration depth. Used for bounce direction determination.
 *
 * @param cx Circle center x coordinate
 * @param cy Circle center y coordinate
 * @param radius Circle radius
 * @param box The collision box
 * @return CollisionSide indicating which side(s) were hit
 */
inline CollisionSide getCircleBoxCollisionSide(int cx, int cy, int radius, const CollisionBox& box)
{
    CollisionSide side;

    // Calculate penetration from each side
    int penetrationLeft = (cx + radius) - box.x;
    int penetrationRight = (box.x + box.w) - (cx - radius);
    int penetrationTop = (cy + radius) - box.y;
    int penetrationBottom = (box.y + box.h) - (cy - radius);

    // Find minimum penetration on each axis
    int minPenetrationX = (penetrationLeft < penetrationRight) ? penetrationLeft : penetrationRight;
    int minPenetrationY = (penetrationTop < penetrationBottom) ? penetrationTop : penetrationBottom;

    // Determine collision side based on minimum penetration
    if (minPenetrationX < minPenetrationY)
    {
        // Horizontal collision (shallower penetration)
        side.x = (penetrationLeft < penetrationRight) ? SIDE_LEFT : SIDE_RIGHT;
    }
    else if (minPenetrationY < minPenetrationX)
    {
        // Vertical collision (shallower penetration)
        side.y = (penetrationTop < penetrationBottom) ? SIDE_TOP : SIDE_BOTTOM;
    }
    else
    {
        // Corner hit (equal penetration on both axes)
        side.x = (penetrationLeft < penetrationRight) ? SIDE_LEFT : SIDE_RIGHT;
        side.y = (penetrationTop < penetrationBottom) ? SIDE_TOP : SIDE_BOTTOM;
    }

    return side;
}
