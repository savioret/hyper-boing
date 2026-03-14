#pragma once

#include <vector>
#include <memory>
#include <climits>
#include <string>
#include "../entities/pickuptype.h"  // For PickupType, DeathPickupEntry (lightweight, no SDL)
#include "glass.h"                   // For GlassType enum
#include "floor.h"                   // For FloorType enum

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
    float dirX = 1.0f;
    int dirY = 1;
    int ballType = 0;
    DeathPickupEntry deathPickups[MAX_DEATH_PICKUPS] = {};
    int deathPickupCount = 0;

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
        // dirX is float; any finite value is valid
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
    FloorType floorType  = FloorType::HORIZ_BIG;
    int floorColor   = 0;      ///< Color index: 0=red, 1=blue, 2=green, 3=yellow
    bool invisible   = false;  ///< Hidden until revealed by a weapon shot
    bool passthrough = false;  ///< Player can walk through horizontally

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
    GlassType glassType    = GlassType::HORIZ_BIG;
    int glassColor         = 0;    ///< Color index: 0=red, 1=blue, 2=green, 3=yellow
    bool hasDeathPickup    = false;
    PickupType deathPickupType = PickupType::GUN;
    bool invisible   = false;  ///< Hidden until revealed by a weapon shot
    bool passthrough = false;  ///< Player can walk through horizontally

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
 * HexaParams struct
 *
 * Type-safe parameters for hexa (hexagon) enemy objects.
 *
 * Fields:
 * - size: Hexa size (0=largest, 2=smallest) - only 3 sizes unlike Ball's 4
 * - velX: Horizontal velocity (constant, not direction-based)
 * - velY: Vertical velocity (constant, not direction-based)
 */
struct HexaParams : public StageObjectParams
{
    int size = 0;           // 0-2 (smaller range than Ball's 0-3)
    int hexaColor = 0;      // 0=green, 1=cyan, 2=orange, 3=purple
    float velX = 1.5f;      // Horizontal velocity
    float velY = 1.0f;      // Vertical velocity
    DeathPickupEntry deathPickups[MAX_DEATH_PICKUPS] = {};
    int deathPickupCount = 0;

    HexaParams() = default;

    std::unique_ptr<StageObjectParams> clone() const override
    {
        return std::make_unique<HexaParams>(*this);
    }

    /**
     * Validate hexa parameters
     * @return true if all parameters are within valid ranges
     */
    bool validate() const
    {
        return size >= 0 && size <= 2;
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
    Glass  = 8,
    Hexa   = 9
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
    int ypos[2]; // initial Y positions for players 1 and 2 (default: MAX_Y)
    std::string stageFile;  ///< Path to .stg file (set by AppData::initStages, loaded by Scene::init)
    bool skipFileReload = false;  ///< Set by Editor→Scene transition; cleared in Scene::init()
                   
private:
    std::vector<std::unique_ptr<StageObject>> sequence;
    size_t sequenceIndex;  // Track current position for pop()
    int itemsleft;  // Number of balls remaining in the stage

public:
    Stage() : id(0), timelimit(0), itemsleft(0), sequenceIndex(0) {
        xpos[0] = xpos[1] = 0;
        ypos[0] = ypos[1] = MAX_Y;
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
     * Get read-only access to the object sequence (for Editor/Save)
     */
    const std::vector<std::unique_ptr<StageObject>>& getSequence() const { return sequence; }

    /**
     * Replace the entire object sequence (used by Editor when saving back)
     */
    void replaceSequence(std::vector<std::unique_ptr<StageObject>>&& newSeq)
    {
        sequence = std::move(newSeq);
        sequenceIndex = 0;
        countItemsLeft();
    }

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
     * Create a hexa (hexagon enemy) object builder
     */
    static StageObjectBuilder hexa()
    {
        return StageObjectBuilder(StageObjectType::Hexa, std::make_unique<HexaParams>());
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
     * Set ball/hexa size
     * @param size Ball size (0=largest, 3=smallest) or Hexa size (0=largest, 2=smallest)
     */
    StageObjectBuilder& size(int size)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            ball->size = size;
        }
        else if (auto* hexa = dynamic_cast<HexaParams*>(objectParams.get()))
        {
            hexa->size = size;
        }
        return *this;
    }

    /**
     * Set hexa velocity (hexa objects only)
     * @param vx Horizontal velocity
     * @param vy Vertical velocity
     */
    StageObjectBuilder& velocity(float vx, float vy)
    {
        if (auto* hexa = dynamic_cast<HexaParams*>(objectParams.get()))
        {
            hexa->velX = vx;
            hexa->velY = vy;
        }
        return *this;
    }

    /**
     * Set color (hexa: 0=green/1=cyan/2=orange/3=purple;
     *            floor: 0=red/1=blue/2=green/3=yellow;
     *            glass: 0=red/1=green/2=yellow)
     */
    StageObjectBuilder& color(int c)
    {
        if (auto* hexa = dynamic_cast<HexaParams*>(objectParams.get()))
            hexa->hexaColor = c;
        else if (auto* floor = dynamic_cast<FloorParams*>(objectParams.get()))
            floor->floorColor = c;
        else if (auto* glass = dynamic_cast<GlassParams*>(objectParams.get()))
            glass->glassColor = c;
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
    StageObjectBuilder& dir(float dirX, int dirY)
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
     * Set a pickup to spawn when the object dies at its own (current) size.
     * - For ball/hexa: adds an entry bound to the object's configured size.
     * - For glass: sets the single death pickup (no size splitting for glass).
     */
    StageObjectBuilder& deathPickup(PickupType type)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            if (ball->deathPickupCount < MAX_DEATH_PICKUPS)
                ball->deathPickups[ball->deathPickupCount++] = { ball->size, type };
        }
        else if (auto* hexa = dynamic_cast<HexaParams*>(objectParams.get()))
        {
            if (hexa->deathPickupCount < MAX_DEATH_PICKUPS)
                hexa->deathPickups[hexa->deathPickupCount++] = { hexa->size, type };
        }
        else if (auto* glass = dynamic_cast<GlassParams*>(objectParams.get()))
        {
            glass->hasDeathPickup = true;
            glass->deathPickupType = type;
        }
        return *this;
    }

    /**
     * Set invisible flag (hidden until revealed by a weapon shot).
     * Applies to Floor and Glass objects.
     */
    StageObjectBuilder& invisible(bool v = true)
    {
        if (auto* floor = dynamic_cast<FloorParams*>(objectParams.get()))
            floor->invisible = v;
        else if (auto* glass = dynamic_cast<GlassParams*>(objectParams.get()))
            glass->invisible = v;
        return *this;
    }

    /**
     * Set passthrough flag (player can walk through horizontally).
     * Applies to Floor and Glass objects.
     */
    StageObjectBuilder& passthrough(bool v = true)
    {
        if (auto* floor = dynamic_cast<FloorParams*>(objectParams.get()))
            floor->passthrough = v;
        else if (auto* glass = dynamic_cast<GlassParams*>(objectParams.get()))
            glass->passthrough = v;
        return *this;
    }

    /**
     * Set a pickup to spawn when this object (or a descendant) is killed at a
     * specific size.  When the current ball/hexa is killed at a higher size
     * (smaller number), the entry is propagated to one random child.
     *
     * @param sz   Size at which to spawn the pickup (0=largest)
     * @param type Pickup type to spawn
     */
    StageObjectBuilder& sizedDeathPickup(int sz, PickupType type)
    {
        if (auto* ball = dynamic_cast<BallParams*>(objectParams.get()))
        {
            if (ball->deathPickupCount < MAX_DEATH_PICKUPS)
                ball->deathPickups[ball->deathPickupCount++] = { sz, type };
        }
        else if (auto* hexa = dynamic_cast<HexaParams*>(objectParams.get()))
        {
            if (hexa->deathPickupCount < MAX_DEATH_PICKUPS)
                hexa->deathPickups[hexa->deathPickupCount++] = { sz, type };
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