#include "stageloader.h"
#include "stage.h"
#include "glass.h"
#include "logger.h"
#include "main.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

std::string StageLoader::trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";

    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

int StageLoader::getIndentLevel(const std::string& line)
{
    int indent = 0;
    for (char c : line)
    {
        if (c == ' ')
            indent++;
        else if (c == '\t')
            indent += 4; // Treat tab as 4 spaces
        else
            break;
    }
    return indent;
}

float StageLoader::parseTimeBlock(const std::string& line)
{
    // Format: "at <time>:"
    // Extract time value (integer or float)
    if (line.length() < 4) // "at X:" minimum
        return -1.0f;

    // Find position after "at "
    size_t start = 3; // Skip "at "
    size_t end = line.find(':', start);
    if (end == std::string::npos)
        return -1.0f;

    std::string timeStr = trim(line.substr(start, end - start));

    try
    {
        return std::stof(timeStr);
    }
    catch (...)
    {
        LOG_ERROR("Invalid time value: %s", timeStr.c_str());
        return 0.0f;
    }
}

bool StageLoader::parseStageProperty(Stage& stage, const std::string& key, const std::string& value)
{
    if (key == "stage_id")
        stage.id = std::stoi(value);
    else if (key == "background")
        stage.setBack(value.c_str());
    else if (key == "music")
        stage.setMusic(value.c_str());
    else if (key == "time_limit")
        stage.timelimit = std::stoi(value);
    else if (key == "player1_x")
        stage.xpos[AppData::PLAYER1] = std::stoi(value);
    else if (key == "player2_x")
        stage.xpos[AppData::PLAYER2] = std::stoi(value);
    else
        return false;

    return true;
}

std::map<std::string, std::string> StageLoader::parseParams(const std::string& paramString)
{
    std::map<std::string, std::string> result;

    // Support both comma-separated and space-separated: "x=100, y=200" or "x=100 y=200"
    // Commas inside [...] (e.g. pickup tables) are preserved.
    std::string normalized = paramString;
    int depth = 0;
    for (char& c : normalized)
    {
        if (c == '[')
            depth++;
        else if (c == ']')
            depth--;
        else if (c == ',' && depth == 0)
            c = ' ';
    }

    std::istringstream iss(normalized);
    std::string token;

    while (iss >> token)
    {
        size_t eqPos = token.find('=');
        if (eqPos != std::string::npos)
        {
            std::string key = token.substr(0, eqPos);
            std::string value = token.substr(eqPos + 1);
            result[key] = value;
        }
        else
        {
            // Handle boolean flags like "y_max" (no =value means true)
            result[token] = "true";
        }
    }

    return result;
}

bool StageLoader::parseObjectLine(Stage& stage, float currentTime, const std::string& line)
{
    // Format: "ball: x=100, y=200, size=2" or "floor: x=550 y=50 type=0"
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos)
        return false;

    std::string objectType = trim(line.substr(0, colonPos));
    std::string paramString = trim(line.substr(colonPos + 1));

    auto params = parseParams(paramString);

    if (objectType == "ball")
    {
        processBallObject(stage, currentTime, params);
    }
    else if (objectType == "floor")
    {
        processFloorObject(stage, currentTime, params);
    }
    else if (objectType == "ladder")
    {
        processLadderObject(stage, currentTime, params);
    }
    else if (objectType == "pickup")
    {
        processPickupObject(stage, currentTime, params);
    }
    else if (objectType == "glass")
    {
        processGlassObject(stage, currentTime, params);
    }
    else if (objectType == "hexa")
    {
        processHexaObject(stage, currentTime, params);
    }
    else
    {
        LOG_WARNING("Unknown object type: %s", objectType.c_str());
        return false;
    }

    return true;
}

bool StageLoader::parseActionLine(Stage& stage, float currentTime, const std::string& line)
{
    // Format: "/weapon gun 1" - remove leading /
    std::string command = trim(line.substr(1));

    if (command.empty())
        return false;

    processActionObject(stage, currentTime, command);
    return true;
}

bool StageLoader::parsePickupTypeString(const std::string& str, PickupType& out)
{
    if (str == "gun")             { out = PickupType::GUN;          return true; }
    if (str == "doubleshoot")     { out = PickupType::DOUBLE_SHOOT; return true; }
    if (str == "extratime")       { out = PickupType::EXTRA_TIME;   return true; }
    if (str == "timefreeze")      { out = PickupType::TIME_FREEZE;  return true; }
    if (str == "1up" || str == "extralife") { out = PickupType::EXTRA_LIFE; return true; }
    if (str == "shield")          { out = PickupType::SHIELD;       return true; }
    if (str == "claw")            { out = PickupType::CLAW;         return true; }
    LOG_WARNING("Unknown pickup type: %s", str.c_str());
    return false;
}

void StageLoader::applyPickupParam(const std::string& value, StageObjectBuilder& builder)
{
    if (value.empty())
        return;

    if (value.front() == '[')
    {
        // Size-keyed pickup table: [0:gun,1:doubleshoot,2:extratime]
        // Strip surrounding brackets
        size_t closePos = value.rfind(']');
        std::string inner = (closePos != std::string::npos)
                            ? value.substr(1, closePos - 1)
                            : value.substr(1);

        // Split by comma
        std::istringstream ss(inner);
        std::string entry;
        while (std::getline(ss, entry, ','))
        {
            entry = trim(entry);
            if (entry.empty())
                continue;

            size_t colonPos = entry.find(':');
            if (colonPos == std::string::npos)
            {
                LOG_WARNING("Pickup table entry missing ':': %s", entry.c_str());
                continue;
            }

            int sz = 0;
            try { sz = std::stoi(entry.substr(0, colonPos)); }
            catch (...) { LOG_WARNING("Invalid pickup size in entry: %s", entry.c_str()); continue; }

            std::string typeName = trim(entry.substr(colonPos + 1));
            PickupType pt;
            if (parsePickupTypeString(typeName, pt))
                builder.withSizedDeathPickup(sz, pt);
        }
    }
    else
    {
        // Legacy single type: applies to the ball/hexa's own configured size
        PickupType pt;
        if (parsePickupTypeString(value, pt))
            builder.withDeathPickup(pt);
    }
}

void StageLoader::processBallObject(Stage& stage, float time, const std::map<std::string, std::string>& params)
{
    // Use StageObjectBuilder for consistency
    auto builder = StageObjectBuilder::ball();

    // Set spawn time
    builder.time(static_cast<int>(time));

    // Position handling (INT_MAX defaults)
    // Priority: y_max > both x+y > individual x or y
    if (params.count("y_max"))
    {
        // Equivalent to .atMaxY() - sets Y=22, X remains random
        builder.atMaxY();
    }
    else if (params.count("x") && params.count("y"))
    {
        // Equivalent to .at(x, y) - fixed position
        builder.at(std::stoi(params.at("x")), std::stoi(params.at("y")));
    }
    else if (params.count("x"))
    {
        // Equivalent to .atX(x) - fixed X, random Y
        builder.atX(std::stoi(params.at("x")));
    }
    else if (params.count("y"))
    {
        // Equivalent to .atY(y) - random X, fixed Y
        builder.atY(std::stoi(params.at("y")));
    }
    // If no position specified, both remain INT_MAX (fully random)

    // Ball-specific properties
    if (params.count("size"))
        builder.size(std::stoi(params.at("size")));
    if (params.count("top"))
        builder.top(std::stoi(params.at("top")));
    if (params.count("dirX") || params.count("dirY"))
    {
        float dx = params.count("dirX") ? std::stof(params.at("dirX")) : 1.0f;
        int   dy = params.count("dirY") ? std::stoi(params.at("dirY")) : 1;
        builder.dir(dx, dy);
    }
    if (params.count("type"))
        builder.type(std::stoi(params.at("type")));
    if (params.count("pickup"))
        applyPickupParam(params.at("pickup"), builder);

    stage.spawn(builder);
}

void StageLoader::processFloorObject(Stage& stage, float time, const std::map<std::string, std::string>& params)
{
    // Parse floor type from string (same naming convention as Glass)
    FloorType type = FloorType::HORIZ_BIG;  // default
    if (params.count("type"))
    {
        const std::string& t = params.at("type");
        if (t == "vert_big")
            type = FloorType::VERT_BIG;
        else if (t == "vert_middle")
            type = FloorType::VERT_MIDDLE;
        else if (t == "horiz_big")
            type = FloorType::HORIZ_BIG;
        else if (t == "horiz_middle")
            type = FloorType::HORIZ_MIDDLE;
        else if (t == "small")
            type = FloorType::SMALL;
        else
            LOG_WARNING("Unknown floor type: %s, defaulting to horiz_big", t.c_str());
    }

    auto builder = StageObjectBuilder::floor();
    builder.time(static_cast<int>(time));
    builder.type(static_cast<int>(type));

    if (params.count("x") && params.count("y"))
        builder.at(std::stoi(params.at("x")), std::stoi(params.at("y")));

    stage.spawn(builder);
}

void StageLoader::processLadderObject(Stage& stage, float time, const std::map<std::string, std::string>& params)
{
    // Use StageObjectBuilder for consistency
    auto builder = StageObjectBuilder::ladder();

    // Set spawn time
    builder.time(static_cast<int>(time));

    // Position (ladders use bottom-middle coordinates)
    if (params.count("x") && params.count("y"))
        builder.at(std::stoi(params.at("x")), std::stoi(params.at("y")));

    // Ladder-specific properties
    if (params.count("height"))
        builder.height(std::stoi(params.at("height")));

    stage.spawn(builder);
}

void StageLoader::processPickupObject(Stage& stage, float time, const std::map<std::string, std::string>& params)
{
    // Parse pickup type from string
    PickupType type = PickupType::GUN;  // Default
    if (params.count("type"))
    {
        const std::string& typeStr = params.at("type");
        if (typeStr == "gun")
            type = PickupType::GUN;
        else if (typeStr == "doubleshoot")
            type = PickupType::DOUBLE_SHOOT;
        else if (typeStr == "extratime")
            type = PickupType::EXTRA_TIME;
        else if (typeStr == "timefreeze")
            type = PickupType::TIME_FREEZE;
        else if (typeStr == "1up" || typeStr == "extralife")
            type = PickupType::EXTRA_LIFE;
        else if (typeStr == "shield")
            type = PickupType::SHIELD;
        else if (typeStr == "claw")
            type = PickupType::CLAW;
        else
        {
            LOG_WARNING("Unknown pickup type: %s, defaulting to gun", typeStr.c_str());
        }
    }

    // Use StageObjectBuilder for consistency
    auto builder = StageObjectBuilder::pickup(type);

    // Set spawn time
    builder.time(static_cast<int>(time));

    // Position (pickups always use fixed coordinates)
    if (params.count("x") && params.count("y"))
        builder.at(std::stoi(params.at("x")), std::stoi(params.at("y")));

    stage.spawn(builder);
}

void StageLoader::processGlassObject(Stage& stage, float time, const std::map<std::string, std::string>& params)
{
    // Parse glass type from string
    GlassType type = GlassType::HORIZ_BIG;  // default
    if (params.count("type"))
    {
        const std::string& t = params.at("type");
        if (t == "vert_big")
            type = GlassType::VERT_BIG;
        else if (t == "vert_middle")
            type = GlassType::VERT_MIDDLE;
        else if (t == "horiz_big")
            type = GlassType::HORIZ_BIG;
        else if (t == "horiz_middle")
            type = GlassType::HORIZ_MIDDLE;
        else if (t == "small")
            type = GlassType::SMALL;
        else
            LOG_WARNING("Unknown glass type: %s, defaulting to horiz_big", t.c_str());
    }

    auto builder = StageObjectBuilder::glass(type);
    builder.time(static_cast<int>(time));

    if (params.count("x") && params.count("y"))
        builder.at(std::stoi(params.at("x")), std::stoi(params.at("y")));
    if (params.count("pickup"))
    {
        PickupType pt;
        if (parsePickupTypeString(params.at("pickup"), pt))
            builder.withDeathPickup(pt);
    }

    stage.spawn(builder);
}

void StageLoader::processHexaObject(Stage& stage, float time, const std::map<std::string, std::string>& params)
{
    auto builder = StageObjectBuilder::hexa();
    builder.time(static_cast<int>(time));

    // Position
    if (params.count("x") && params.count("y"))
        builder.at(std::stoi(params.at("x")), std::stoi(params.at("y")));

    // Size (0-2 for hexa, unlike ball's 0-3)
    if (params.count("size"))
        builder.size(std::stoi(params.at("size")));

    // Velocity (constant velocity, not direction-based)
    float velX = 1.5f;
    float velY = 1.0f;
    if (params.count("velX"))
        velX = std::stof(params.at("velX"));
    if (params.count("velY"))
        velY = std::stof(params.at("velY"));
    builder.velocity(velX, velY);

    // Death pickup
    if (params.count("pickup"))
        applyPickupParam(params.at("pickup"), builder);

    stage.spawn(builder);
}

void StageLoader::processActionObject(Stage& stage, float time, const std::string& command)
{
    auto builder = StageObjectBuilder::action(command);
    builder.time(static_cast<int>(time));
    stage.spawn(builder);
}

bool StageLoader::load(Stage& stage, const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open stage file: %s", filename.c_str());
        return false;
    }

    stage.reset();

    std::string line;
    float currentTime = -1.0f; // -1 = in header section (before any "at" block)

    while (std::getline(file, line))
    {
        // Skip empty lines and comments
        std::string trimmedLine = trim(line);
        if (trimmedLine.empty() || trimmedLine[0] == '#')
            continue;

        // Check for time block header: "at <time>:"
        if (trimmedLine.rfind("at ", 0) == 0 && trimmedLine.back() == ':')
        {
            currentTime = parseTimeBlock(trimmedLine);
            continue;
        }

        // If we're in header section (before any "at" block)
        if (currentTime < 0)
        {
            // Parse stage-level properties
            size_t colonPos = trimmedLine.find(':');
            if (colonPos != std::string::npos)
            {
                std::string key = trim(trimmedLine.substr(0, colonPos));
                std::string value = trim(trimmedLine.substr(colonPos + 1));
                parseStageProperty(stage, key, value);
            }
            continue;
        }

        // Inside a time block - parse action or object
        if (trimmedLine[0] == '/')
        {
            parseActionLine(stage, currentTime, trimmedLine);
        }
        else
        {
            // Parse object definition
            parseObjectLine(stage, currentTime, trimmedLine);
        }
    }

    file.close();
    
    // Count balls after loading all objects from file
    stage.countItemsLeft();
    
    LOG_INFO("Loaded stage from file: %s", filename.c_str());
    return true;
}

bool StageLoader::save(const Stage& stage, const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open())
        return false;

    // Write stage properties
    file << "# Stage " << stage.id << "\n";
    file << "stage_id: " << stage.id << "\n";
    file << "background: " << stage.back << "\n";
    file << "music: " << stage.music << "\n";
    file << "time_limit: " << stage.timelimit << "\n";
    file << "player1_x: " << stage.xpos[AppData::PLAYER1] << "\n";
    file << "player2_x: " << stage.xpos[AppData::PLAYER2] << "\n";
    file << "\n";

    // Note: Saving the sequence would require exposing it or iterating through
    // For now, this is a basic implementation that saves stage metadata
    // Full sequence serialization could be added if needed

    file << "# Objects would be written here\n";
    file << "# This is a placeholder - full serialization requires sequence access\n";

    file.close();
    return true;
}
