#include "stageloader.h"
#include "stage.h"
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

bool StageLoader::parseKeyValue(const std::string& line, std::string& key, std::string& value)
{
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos)
        return false;
    
    key = trim(line.substr(0, colonPos));
    value = trim(line.substr(colonPos + 1));
    return true;
}

void StageLoader::processBallObject(Stage& stage, const std::map<std::string, std::string>& objectData)
{
    // Use StageObjectBuilder for consistency with appdata.cpp
    auto builder = StageObjectBuilder::ball();
    
    // Position handling (INT_MAX defaults)
    // Priority: y_max > both x+y > individual x or y
    if (objectData.count("y_max"))
    {
        // Equivalent to .atMaxY() - sets Y=22, X remains random
        builder.atMaxY();
    }
    else if (objectData.count("x") && objectData.count("y"))
    {
        // Equivalent to .at(x, y) - fixed position
        builder.at(std::stoi(objectData.at("x")), std::stoi(objectData.at("y")));
    }
    else if (objectData.count("x"))
    {
        // Equivalent to .atX(x) - fixed X, random Y
        builder.atX(std::stoi(objectData.at("x")));
    }
    else if (objectData.count("y"))
    {
        // Equivalent to .atY(y) - random X, fixed Y
        builder.atY(std::stoi(objectData.at("y")));
    }
    // If no position specified, both remain INT_MAX (fully random)
    
    // Timing
    if (objectData.count("start"))
        builder.time(std::stoi(objectData.at("start")));
    
    // Ball-specific properties
    if (objectData.count("size"))
        builder.size(std::stoi(objectData.at("size")));
    if (objectData.count("top"))
        builder.top(std::stoi(objectData.at("top")));
    if (objectData.count("dirX") && objectData.count("dirY"))
        builder.dir(std::stoi(objectData.at("dirX")), std::stoi(objectData.at("dirY")));
    if (objectData.count("ball_type"))
        builder.type(std::stoi(objectData.at("ball_type")));
    
    stage.spawn(builder);
}

void StageLoader::processFloorObject(Stage& stage, const std::map<std::string, std::string>& objectData)
{
    // Use StageObjectBuilder for consistency
    auto builder = StageObjectBuilder::floor();
    
    // Position (floors always use fixed coordinates)
    if (objectData.count("x") && objectData.count("y"))
        builder.at(std::stoi(objectData.at("x")), std::stoi(objectData.at("y")));
    
    // Timing
    if (objectData.count("start"))
        builder.time(std::stoi(objectData.at("start")));
    
    // Floor-specific properties
    if (objectData.count("floor_type"))
        builder.type(std::stoi(objectData.at("floor_type")));
    
    stage.spawn(builder);
}

bool StageLoader::load(Stage& stage, const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open stage file: %s", filename);
        return false;
    }
    
    stage.reset();
    
    std::string line;
    std::string currentSection;
    std::map<std::string, std::string> objectData;
    
    while (std::getline(file, line))
    {
        line = trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#')
            continue;
        
        std::string key, value;
        
        // Check for new object section
        if (line == "ball:" || line == "floor:")
        {
            // Process previous object if any
            if (!currentSection.empty() && !objectData.empty())
            {
                if (currentSection == "ball")
                    processBallObject(stage, objectData);
                else if (currentSection == "floor")
                    processFloorObject(stage, objectData);
                
                objectData.clear();
            }
            
            // Start new section
            currentSection = line.substr(0, line.length() - 1);
            continue;
        }
        
        // Parse key-value pairs
        if (parseKeyValue(line, key, value))
        {
            if (currentSection.empty())
            {
                // Stage-level properties
                if (key == "stage_id")
                    stage.id = std::stoi(value);
                else if (key == "background")
                    stage.setBack(value.c_str());
                else if (key == "music")
                    stage.setMusic(value.c_str());
                else if (key == "time_limit")
                    stage.timelimit = std::stoi(value);
                else if (key == "player1_x")
                    stage.xpos[PLAYER1] = std::stoi(value);
                else if (key == "player2_x")
                    stage.xpos[PLAYER2] = std::stoi(value);
            }
            else
            {
                // Object properties
                objectData[key] = value;
            }
        }
    }
    
    // Process last object if any
    if (!currentSection.empty() && !objectData.empty())
    {
        if (currentSection == "ball")
            processBallObject(stage, objectData);
        else if (currentSection == "floor")
            processFloorObject(stage, objectData);
    }
    
    file.close();
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
    file << "player1_x: " << stage.xpos[PLAYER1] << "\n";
    file << "player2_x: " << stage.xpos[PLAYER2] << "\n";
    file << "\n";
    
    // Note: Saving the sequence would require exposing it or iterating through
    // For now, this is a basic implementation that saves stage metadata
    // Full sequence serialization could be added if needed
    
    file << "# Objects would be written here\n";
    file << "# This is a placeholder - full serialization requires sequence access\n";
    
    file.close();
    return true;
}
