#pragma once

#include <vector>
#include <memory>
#include <climits>
#include <string>
#include "../entities/pickup.h"  // For PickupType enum
#include "glass.h"               // For GlassType enum
#include "floor.h"               // For FloorType enum

/**
 * StageObjectParams base class
 *
 * Base class for type-specific parameters of stage objects.
 * Contains common fields like position and spawn timing.
 * 
 * Coordinates initialized to INT_MAX indicate random positioning:
 * - x = INT_MAX: Random X position (calculated at pop time)
 * - y = INT_MAX: Random Y position (or default based on object type)
 */
struct StageObjectParams
{
    int x = INT_MAX;
    int y = INT_MAX;
    int startTime = 0;
    
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
    bool hasDeathPickup = false;
    PickupType deathPickupType = PickupType::GUN;

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
 * - floorType: Shape variant (FloorType enum, maps directly to frame index)
 */
struct FloorParams : public StageObjectParams
{
    FloorType floorType = FloorType::HORIZ_BIG;

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
        int v = static_cast<int>(floorType);
        return v >= 0 && v <= 4;
    }
};

/**
 * LadderParams struct
 *
 * Type-safe parameters for ladder objects.
 *
 * Fields:
 * - numTiles: Number of vertical tiles (height of ladder)
 */
struct LadderParams : public StageObjectParams
{
    int numTiles = 3;  // Default 3 tiles high

    LadderParams() = default;

    std::unique_ptr<StageObjectParams> clone() const override
    {
        return std::make_unique<LadderParams>(*this);
    }

    /**
     * Validate ladder parameters
     * @return true if all parameters are within valid ranges
     */
    bool validate() const
    {
        return numTiles > 0;
    }
};

/**
 * ActionParams struct
 *
 * Parameters for action objects (console commands executed at spawn time).
 *
 * Fields:
 * - command: Console command string to execute (without leading /)
 */
struct ActionParams : public StageObjectParams
{
    std::string command;

    ActionParams() = default;

    std::unique_ptr<StageObjectParams> clone() const override
    {
        return std::make_unique<ActionParams>(*this);
    }

    bool validate() const
    {
        return !command.empty();
    }
};

/**
 * PickupParams struct
 *
 * Type-safe parameters for pickup/powerup objects.
 *
 * Fields:
 * - pickupType: Type of pickup (GUN, DOUBLE_SHOOT, etc.)
 */
struct PickupParams : public StageObjectParams
{
    PickupType pickupType = PickupType::GUN;

    PickupParams() = default;

    std::unique_ptr<StageObjectParams> clone() const override
    {
        return std::make_unique<PickupParams>(*this);
    }

    /**
     * Validate pickup parameters
     * @return true if all parameters are within valid ranges
     */
    bool validate() const
    {
        int typeInt = static_cast<int>(pickupType);
        return typeInt >= 0 && typeInt <= 6;
    }
};

/**
 * GlassParams struct
 *
 * Type-safe parameters for glass platform objects.
 *
 * Fields:
 * - glassType: Shape variant (VERT_BIG, VERT_MIDDLE, HORIZ_BIG, HORIZ_MIDDLE, SMALL)
 */
struct GlassParams : public StageObjectParams
{
    GlassType glassType = GlassType::HORIZ_BIG;
    bool hasDeathPickup = false;
    PickupType deathPickupType = PickupType::GUN;

    GlassParams() = default;

    std::unique_ptr<StageObjectParams> clone() const override
    {
        return std::make_unique<GlassParams>(*this);
    }

    bool validate() const
    {
        int t = static_cast<int>(glassType);
        return t == 0 || t == 5 || t == 10 || t == 15 || t == 20;
    }
};

/**
 * Identifies the type of a stage spawn object.
 * Used in StageObject::id and the Scene spawn switch.
 */
enum class StageObjectType : int
{
    Null   = 0,
    Ball   = 1,
    Shoot  = 2,
    Floor  = 3,
    Item   = 4,
    Player = 5,
    Action = 6,
    Ladder = 7,
    Glass  = 8
};

/**
 * StageObject struct
 * Stores information about an object: position, ID, and when it appears.
 */
struct StageObject
{
    StageObjectType id;
    int start; // start time
    int x, y;

    // Type-safe parameters
    std::unique_ptr<StageObjectParams> params;

    StageObject(StageObjectType id = StageObjectType::Null, int start = 0)
        : id(id), start(start), x(INT_MAX), y(INT_MAX), params(nullptr)
    {
    }

    // Constructor with typed params
    StageObject(StageObjectType id, std::unique_ptr<StageObjectParams> p)
        : id(id), start(p ? p->startTime : 0), x(p ? p->x : INT_MAX), y(p ? p->y : INT_MAX),
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
    StageObject(StageObject&& other) noexcept
        : id(other.id), start(other.start), x(other.x), y(other.y), params(std::move(other.params))
    {
    }

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

    StageObject& operator=(StageObject&& other) noexcept
    {
        if (this != &other)
        {
            id = other.id;
            start = other.start;
            x = other.x;
            y = other.y;
            params = std::move(other.params);
        }
        return *this;
    }

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
    // Playfield boundaries
    static constexpr int MAX_Y = 415;
    static constexpr int MIN_Y = 16;
    static constexpr int MAX_X = 623;
    static constexpr int MIN_X = 16;

    int id;
    char back[64];
    char music[64];
    int timelimit;
    int xpos[2]; // initial positions for players 1 and 2 TODO: Review this
                   
private:
    std::vector<std::unique_ptr<StageObject>> sequence;
    size_t sequenceIndex;  // Track current position for pop()
    int itemsleft;  // Number of balls remaining in the stage

public:
    Stage() : id(0), timelimit(0), itemsleft(0), sequenceIndex(0) {
        xpos[0] = xpos[1] = 0;
        back[0] = '\0';
        music[0] = '\0';
    }

    /**
     * Reset stage data and state
     * Clears all objects from the sequence and resets counters.
     */
    void reset();

    /**
     * Reset sequence playback to beginning
     * Called when restarting a stage (e.g., via /goto command or replaying)
     */
    void restart();
    
    /**
     * Count and update the number of ball items in the sequence
     * Should be called after populating the stage with spawn() calls
     */
    void countItemsLeft();
    
    /**
     * Get the number of balls remaining in the stage
     * @return Number of balls left to clear
     */
    int getItemsLeft() const { return itemsleft; }

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
     * @param obj StageObject with typed params (moved)
     */
    void spawn(StageObject&& obj);
    
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
     * Calculates random positions for INT_MAX coordinates
     * Validates ball positions against floor collisions
     * @param time Current game time
     * @return StageObject (id=StageObjectType::Null if nothing to pop)
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
 * Coordinates default to INT_MAX (random positioning):
 * - Use .at(x, y) to set both coordinates explicitly
 * - Use .atX(x) to set only X (Y remains random)
 * - Use .atY(y) to set only Y (X remains random)
 * - Leave both unset for fully random positioning
 * 
 * Example usage:
 *   // Fixed position
 *   stage.spawn(StageObjectBuilder::ball().at(100, 200).time(1).size(2));
 *   
 *   // Random X, fixed Y
 *   stage.spawn(StageObjectBuilder::ball().atY(400).time(1).size(2));
 *   
 *   // Fixed X, random Y
 *   stage.spawn(StageObjectBuilder::ball().atX(300).time(1).size(2));
 *   
 *   // Fully random (default top for bal
 *   stage.spawn(StageObjectBuilder::ball().time(1).size(2));
 */
class StageObjectBuilder
{
private:
    StageObjectType objectId;
    std::unique_ptr<StageObjectParams> objectParams;

    StageObjectBuilder(StageObjectType id, std::unique_ptr<StageObjectParams> params)
        : objectId(id), objectParams(std::move(params))
    {
    }

public:
    /**
     * Create a ball object builder
     */
    static StageObjectBuilder ball()
    {
        return StageObjectBuilder(StageObjectType::Ball, std::make_unique<BallParams>());
    }

    /**
     * Create a floor object builder
     */
    static StageObjectBuilder floor()
    {
        return StageObjectBuilder(StageObjectType::Floor, std::make_unique<FloorParams>());
    }

    /**
     * Create an action object builder
     * @param command Console command to execute (without leading /)
     */
    static StageObjectBuilder action(const std::string& command)
    {
        auto params = std::make_unique<ActionParams>();
        params->command = command;
        return StageObjectBuilder(StageObjectType::Action, std::move(params));
    }

    /**
     * Create a ladder object builder
     */
    static StageObjectBuilder ladder()
    {
        return StageObjectBuilder(StageObjectType::Ladder, std::make_unique<LadderParams>());
    }

    /**
     * Create a pickup object builder
     * @param type Pickup type (default: GUN)
     */
    static StageObjectBuilder pickup(PickupType type = PickupType::GUN)
    {
        auto params = std::make_unique<PickupParams>();
        params->pickupType = type;
        return StageObjectBuilder(StageObjectType::Item, std::move(params));
    }

    /**
     * Create a glass platform object builder
     * @param type Glass shape variant (default: HORIZ_BIG)
     */
    static StageObjectBuilder glass(GlassType type = GlassType::HORIZ_BIG)
    {
        auto params = std::make_unique<GlassParams>();
        params->glassType = type;
        return StageObjectBuilder(StageObjectType::Glass, std::move(params));
    }

    /**
     * Set exact position (both X and Y)
     * @param x X coordinate
     * @param y Y coordinate
     */
    StageObjectBuilder& at(int x, int y)
    {
        objectParams->x = x;
        objectParams->y = y;
        return *this;
    }
    
    /**
     * Set only X coordinate (Y remains INT_MAX for random)
     * @param x X coordinate
     */
    StageObjectBuilder& atX(int x)
    {
        objectParams->x = x;
        return *this;
    }
    
    /**
     * Set only Y coordinate (X remains INT_MAX for random)
     * @param y Y coordinate
     */
    StageObjectBuilder& atY(int y)
    {
        objectParams->y = y;
        return *this;
    }
    
    /**
     * Set Y coordinate to screen top (22px), X remains random
     * Convenience method for balls spawning at top with random X position
     */
    StageObjectBuilder& atMaxY()
    {
        objectParams->y = Stage::MIN_Y + 6;
        return *this;
    }
    
    /**
     * Set spawn time
     * @param time Game tick when object should appear
     */
    StageObjectBuilder& time(int time)
    {
        objectParams->startTime = time;
        return *this;
    }
    
    // Ball-specific methods
    
    /**
     * Set ball size (ball objects only)
     * @param size Ball size (0=largest, 3=smallest)
     */
    StageObjectBuilder& size(int size)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            ball->size = size;
        }
        return *this;
    }
    
    /**
     * Set ball maximum jump height (ball objects only)
     * @param top Maximum height from floor (0=auto-calculate based on size)
     */
    StageObjectBuilder& top(int top)
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
    StageObjectBuilder& dir(int dirX, int dirY)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            ball->dirX = dirX;
            ball->dirY = dirY;
        }
        return *this;
    }

    /**
     * Set ladder height in tiles (ladder objects only)
     * @param tiles Number of vertical tiles
     */
    StageObjectBuilder& height(int tiles)
    {
        if (auto* ladder = dynamic_cast<LadderParams*>(objectParams.get()))
        {
            ladder->numTiles = tiles;
        }
        return *this;
    }

    /**
     * Set pickup type (pickup objects only)
     * @param type Pickup type (GUN, DOUBLE_SHOOT, etc.)
     */
    StageObjectBuilder& pickupType(PickupType type)
    {
        if (auto* pickup = dynamic_cast<PickupParams*>(objectParams.get()))
        {
            pickup->pickupType = type;
        }
        return *this;
    }

    /**
     * Set a pickup to spawn at this object's center when it dies.
     * Supported for ball and glass objects.
     */
    StageObjectBuilder& withDeathPickup(PickupType type)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            ball->hasDeathPickup = true;
            ball->deathPickupType = type;
        }
        else if (auto* glass = dynamic_cast<GlassParams*>(objectParams.get()))
        {
            glass->hasDeathPickup = true;
            glass->deathPickupType = type;
        }
        return *this;
    }

    /**
     * Set object type
     * - For balls: ball type/color (default=0 for red)
     * - For floors: floor type (0=horizontal, 1=vertical)
     */
    StageObjectBuilder& type(int typeValue)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            ball->ballType = typeValue;
        }
        else if (auto* floor = dynamic_cast<FloorParams*>(objectParams.get()))
        {
            floor->floorType = static_cast<FloorType>(typeValue);
        }
        return *this;
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