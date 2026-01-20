#pragma once

#include <list>
#include <memory>

/**
 * SpawnMode enum
 *
 * Defines how objects are positioned when spawned:
 * - FIXED: Use explicit x, y coordinates
 * - RANDOM_X: Random X position (32-632 range), explicit Y
 * - TOP_RANDOM: Random X position at top of screen (y=22)
 */
enum class SpawnMode
{
    FIXED,
    RANDOM_X,
    TOP_RANDOM
};

/**
 * StageObjectParams base class
 *
 * Base class for type-specific parameters of stage objects.
 * Contains common fields like position and spawn timing.
 */
struct StageObjectParams
{
    int x = 0;
    int y = 0;
    int startTime = 0;
    SpawnMode spawnMode = SpawnMode::FIXED;
    
    StageObjectParams() = default;
    virtual ~StageObjectParams() = default;
    
    // Clone for copying StageObject instances
    virtual std::unique_ptr<StageObjectParams> clone() const = 0;
};

/**
 * BallParams struct
 *
 * Type-safe parameters for ball objects with meaningful field names.
 * 
 * Fields:
 * - size: Ball size (0=largest, 3=smallest)
 * - top: Maximum jump height from floor (0=auto-calculate)
 * - dirX: Horizontal direction (-1=left, 1=right)
 * - dirY: Vertical direction (-1=up, 1=down)
 * - ballType: Type/color of ball (default=0 for red)
 */
struct BallParams : public StageObjectParams
{
    int size = 0;
    int top = 0;
    int dirX = 1;
    int dirY = 1;
    int ballType = 0;
    
    BallParams() = default;
    
    std::unique_ptr<StageObjectParams> clone() const override
    {
        return std::make_unique<BallParams>(*this);
    }
    
    /**
     * Validate ball parameters
     * @return true if all parameters are within valid ranges
     */
    bool validate() const
    {
        if (size < 0 || size > 3) return false;
        if (top < 0) return false;
        if (dirX != -1 && dirX != 0 && dirX != 1) return false;
        if (dirY != -1 && dirY != 1) return false;
        if (ballType < 0) return false;
        return true;
    }
};

/**
 * FloorParams struct
 *
 * Type-safe parameters for floor/platform objects.
 * 
 * Fields:
 * - floorType: Type of floor (0=horizontal, 1=vertical)
 */
struct FloorParams : public StageObjectParams
{
    int floorType = 0;
    
    FloorParams() = default;
    
    std::unique_ptr<StageObjectParams> clone() const override
    {
        return std::make_unique<FloorParams>(*this);
    }
    
    /**
     * Validate floor parameters
     * @return true if all parameters are within valid ranges
     */
    bool validate() const
    {
        if (floorType < 0 || floorType > 1) return false;
        return true;
    }
};

/**
 * StageObject struct
 * Stores information about an object: position, ID, and when it appears.
 */
struct StageObject
{
    int id;
    int start; // start time
    int x, y;
    
    // Type-safe parameters
    std::unique_ptr<StageObjectParams> params;

    StageObject(int id = 0, int start = 0)
        : id(id), start(start), x(0), y(0), params(nullptr)
    {
    }
    
    // Constructor with typed params
    StageObject(int id, std::unique_ptr<StageObjectParams> p)
        : id(id), start(p ? p->startTime : 0), x(p ? p->x : 0), y(p ? p->y : 0),
          params(std::move(p))
    {
    }
    
    // Copy constructor (needed for Stage::pop return by value)
    StageObject(const StageObject& other)
        : id(other.id), start(other.start), x(other.x), y(other.y)
    {
        if (other.params)
            params = other.params->clone();
    }
    
    // Move constructor
    StageObject(StageObject&& other) noexcept = default;
    
    // Assignment operators
    StageObject& operator=(const StageObject& other)
    {
        if (this != &other)
        {
            id = other.id;
            start = other.start;
            x = other.x;
            y = other.y;
            if (other.params)
                params = other.params->clone();
            else
                params.reset();
        }
        return *this;
    }
    
    StageObject& operator=(StageObject&& other) noexcept = default;
    
    // Helper to get typed params (returns nullptr if wrong type)
    template<typename T>
    T* getParams()
    {
        return dynamic_cast<T*>(params.get());
    }
    
    template<typename T>
    const T* getParams() const
    {
        return dynamic_cast<const T*>(params.get());
    }
};

/**
 * Stage class
 *
 * This class stores all the information specific to each stage:
 * screen layout, background image, music, time limit, and the
 * list of objects to be spawned.
 */
class Stage
{
public:
    int id;
    char back[64];
    char music[64];
    int timelimit;
    int itemsleft;
    int xpos[2]; // initial positions for players 1 and 2
                   
private:
    std::list<StageObject*> sequence;

public:
    Stage() : id(0), timelimit(0), itemsleft(0) {
        xpos[0] = xpos[1] = 0;
        back[0] = '\0';
        music[0] = '\0';
    }

    /**
     * Reset stage data
     * Clears all objects from the sequence and resets counters.
     */
    void reset();
    
    /**
     * Set background image file
     * @param backFile Path to background image (relative to graph/ folder)
     */
    void setBack(const char* backFile);
    
    /**
     * Set stage music file
     * @param musicFile Path to music file (relative to music/ folder)
     */
    void setMusic(const char* musicFile);
    
    /**
     * Spawn an object with typed parameters
     * @param obj StageObject with typed params
     */
    void spawn(StageObject obj);
    
    /**
     * Spawn a ball at top of screen (random X position, y=22)
     * Convenience method for common use case.
     * @param startTime When to spawn the ball (in game ticks)
     * @param size Ball size (0=largest, 3=smallest, default=0)
     * @param dirX Horizontal direction (-1 or 1, default=1)
     * @param dirY Vertical direction (-1 or 1, default=1)
     */
    void spawnBallAtTop(int startTime, int size = 0, int dirX = 1, int dirY = 1);
    
    /**
     * Spawn a ball with full control
     * @param params BallParams with all configuration
     */
    void spawnBall(const BallParams& params);
    
    /**
     * Spawn a floor/platform
     * @param params FloorParams with all configuration
     */
    void spawnFloor(const FloorParams& params);

    /**
     * Pop next object from sequence if its start time has been reached
     * @param time Current game time
     * @return StageObject (id=OBJ_NULL if nothing to pop)
     */
    StageObject pop(int time);
};

// Forward declaration
class StageObjectBuilder;

/**
 * StageObjectBuilder class
 *
 * Fluent builder API for creating stage objects with readable, chainable syntax.
 * 
 * Example usage:
 *   stage.spawn(StageObjectBuilder::ball()
 *                   .at(100, 200)
 *                   .startsAt(10)
 *                   .withSize(2)
 *                   .withDirection(-1, 1));
 * 
 *   stage.spawnFloor(StageObjectBuilder::floor()
 *                        .at(250, 100)
 *                        .withType(0)
 *                        .startsAt(0));
 */
class StageObjectBuilder
{
private:
    int objectId;
    std::unique_ptr<StageObjectParams> objectParams;
    
    StageObjectBuilder(int id, std::unique_ptr<StageObjectParams> params)
        : objectId(id), objectParams(std::move(params))
    {
    }
    
public:
    /**
     * Create a ball object builder
     */
    static StageObjectBuilder ball()
    {
        return StageObjectBuilder(1, std::make_unique<BallParams>()); // 1 = OBJ_BALL
    }
    
    /**
     * Create a floor object builder
     */
    static StageObjectBuilder floor()
    {
        return StageObjectBuilder(3, std::make_unique<FloorParams>()); // 3 = OBJ_FLOOR
    }
    
    /**
     * Set exact position
     * @param x X coordinate
     * @param y Y coordinate
     */
    StageObjectBuilder& at(int x, int y)
    {
        objectParams->x = x;
        objectParams->y = y;
        objectParams->spawnMode = SpawnMode::FIXED;
        return *this;
    }
    
    /**
     * Spawn at random X position with specified Y
     * @param y Y coordinate
     */
    StageObjectBuilder& atRandomX(int y)
    {
        objectParams->y = y;
        objectParams->spawnMode = SpawnMode::RANDOM_X;
        return *this;
    }
    
    /**
     * Spawn at top of screen with random X (y=22)
     * Common pattern for ball spawning.
     */
    StageObjectBuilder& atTop()
    {
        objectParams->y = 22;
        objectParams->spawnMode = SpawnMode::TOP_RANDOM;
        return *this;
    }
    
    /**
     * Set spawn time
     * @param time Game tick when object should appear
     */
    StageObjectBuilder& startsAt(int time)
    {
        objectParams->startTime = time;
        return *this;
    }
    
    // Ball-specific methods
    
    /**
     * Set ball size (ball objects only)
     * @param size Ball size (0=largest, 3=smallest)
     */
    StageObjectBuilder& withSize(int size)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            ball->size = size;
        }
        return *this;
    }
    
    /**
     * Set ball maximum jump height (ball objects only)
     * @param top Maximum jump height from floor (0=auto-calculate based on size)
	 * @note only working for ball objects
     */
    StageObjectBuilder& withTop(int top)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            ball->top = top;
        }
        return *this;
    }
    
    /**
     * Set ball direction (ball objects only)
     * @param dirX Horizontal direction (-1=left, 1=right)
     * @param dirY Vertical direction (-1=up, 1=down)
     */
    StageObjectBuilder& withDirection(int dirX, int dirY)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            ball->dirX = dirX;
            ball->dirY = dirY;
        }
        return *this;
    }
    
    /**
     * Set ball type/color (ball objects only)
     * @param ballType Type identifier (default=0 for red)
     */
    StageObjectBuilder& withBallType(int ballType)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            ball->ballType = ballType;
        }
        return *this;
    }
    
    // Floor-specific methods
    
    /**
     * Set floor type (floor objects only)
     * @param floorType Floor type (0=horizontal, 1=vertical)
     */
    StageObjectBuilder& withType(int floorType)
    {
        if (auto* floor = dynamic_cast<FloorParams*>(objectParams.get()))
        {
            floor->floorType = floorType;
        }
        return *this;
    }
    
    /**
     * Alias for withType() - set floor type
     */
    StageObjectBuilder& withFloorType(int floorType)
    {
        return withType(floorType);
    }
    
    /**
     * Implicit conversion to StageObject for direct passing to spawn()
     * This allows: stage.spawn(StageObjectBuilder::ball().at(x,y))
     * Without needing to call .build()
     */
    operator StageObject()
    {
        return StageObject(objectId, std::move(objectParams));
    }
    
    /**
     * Convert to FloorParams for direct passing to spawnFloor
     */
    operator FloorParams() const
    {
        if (auto* floor = dynamic_cast<FloorParams*>(objectParams.get()))
        {
            return *floor;
        }
        return FloorParams();
    }
    
    /**
     * Convert to BallParams for direct passing to spawnBall
     */
    operator BallParams() const
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            return *ball;
        }
        return BallParams();
    }
};