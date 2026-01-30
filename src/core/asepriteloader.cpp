#include "asepriteloader.h"
#include "jsonparser.h"
#include "graph.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>

std::string AsepriteLoader::readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open file: %s", path.c_str());
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string AsepriteLoader::getDirectory(const std::string& path)
{
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        return path.substr(0, lastSlash + 1);
    }
    return "";
}

std::unique_ptr<IAnimController> AsepriteLoader::load(
    Graph* graph,
    const std::string& jsonPath,
    SpriteSheet& sheet)
{
    // Read JSON file
    std::string jsonContent = readFile(jsonPath);
    if (jsonContent.empty())
    {
        LOG_ERROR("AsepriteLoader: Failed to read JSON file");
        return nullptr;
    }

    // Parse JSON
    JsonValue root = JsonParser::parse(jsonContent);
    if (!root.isObject())
    {
        LOG_ERROR("AsepriteLoader: Invalid JSON format");
        return nullptr;
    }

    // Get meta section for image path
    if (!root.has("meta"))
    {
        LOG_ERROR("AsepriteLoader: No 'meta' section found");
        return nullptr;
    }

    JsonValue meta = root["meta"];
    if (!meta.has("image"))
    {
        LOG_ERROR("AsepriteLoader: No 'image' field in meta");
        return nullptr;
    }

    // Load texture
    std::string dir = getDirectory(jsonPath);
    std::string imagePath = dir + meta["image"].asString(); 
    
    if (!sheet.init(graph, imagePath))
    {
        LOG_ERROR("AsepriteLoader: Failed to load texture: %s", imagePath.c_str());
        return nullptr;
    }

    // Parse frames
    if (!root.has("frames") || !root["frames"].isArray())
    {
        LOG_ERROR("AsepriteLoader: No 'frames' array found");
        return nullptr;
    }

    JsonValue frames = root["frames"];
    for (size_t i = 0; i < frames.size(); i++)
    {
        JsonValue frameData = frames[i];

        if (!frameData.has("frame") || !frameData.has("spriteSourceSize"))
        {
            LOG_WARNING("AsepriteLoader: Frame %zu missing required data", i);
            continue;
        }

        // Get frame position and size in texture
        JsonValue frame = frameData["frame"];
        int x = frame["x"].asInt();
        int y = frame["y"].asInt();
        int w = frame["w"].asInt();
        int h = frame["h"].asInt();

        // Get sprite source offset (for trimmed sprites)
        JsonValue spriteSource = frameData["spriteSourceSize"];
        int xOff = spriteSource["x"].asInt();
        int yOff = spriteSource["y"].asInt();

        // Get original canvas size (for bottom-middle positioning)
        int srcW = 0, srcH = 0;
        if (frameData.has("sourceSize"))
        {
            JsonValue sourceSize = frameData["sourceSize"];
            srcW = sourceSize["w"].asInt();
            srcH = sourceSize["h"].asInt();
        }

        // Add frame to sprite sheet with sourceSize info
        sheet.addFrame(x, y, w, h, xOff, yOff, srcW, srcH);
    }

    LOG_INFO("AsepriteLoader: Loaded %zu frames from %s", frames.size(), imagePath.c_str());

    int totalFrames = static_cast<int>(frames.size());
    if (totalFrames == 0)
    {
        return nullptr;
    }

    // Collect per-frame durations
    std::vector<int> frameDurations(totalFrames, 100);  // Default 100ms
    bool hasVariableDurations = false;
    
    for (int i = 0; i < totalFrames; i++)
    {
        if (frames[i].has("duration"))
        {
            int duration = frames[i]["duration"].asInt();
            frameDurations[i] = duration;
            if (i > 0 && duration != frameDurations[0])
            {
                hasVariableDurations = true;
            }
        }
    }
    
    // If all durations are the same, use uniform duration for efficiency
    int uniformDuration = frameDurations[0];
    if (!hasVariableDurations)
    {
        LOG_INFO("AsepriteLoader: Using uniform duration of %dms for all frames", uniformDuration);
    }
    else
    {
        LOG_INFO("AsepriteLoader: Using per-frame durations (variable)");
    }

    // Parse frameTags to create animation controller
    if (meta.has("frameTags") && meta["frameTags"].isArray())
    {
        JsonValue frameTags = meta["frameTags"];
        
        if (frameTags.size() > 0)
        {
            // Create state machine with states for tagged and untagged frame ranges
            auto anim = std::make_unique<StateMachineAnim>();
            
            // Track which frames are covered by tags
            std::vector<bool> covered(totalFrames, false);
            
            // Process all tags
            for (size_t tagIdx = 0; tagIdx < frameTags.size(); tagIdx++)
            {
                JsonValue tag = frameTags[tagIdx];
                
                int from = tag["from"].asInt();
                int to = tag["to"].asInt();
                std::string direction = tag.has("direction") ? tag["direction"].asString() : "forward";
                std::string tagName = tag.has("name") ? tag["name"].asString() : ("tag" + std::to_string(tagIdx));
                
                // Mark frames as covered
                for (int i = from; i <= to && i < totalFrames; i++)
                {
                    covered[i] = true;
                }
                
                // Create sequence based on direction
                std::vector<int> sequence;
                std::vector<int> sequenceDurations;
                
                if (direction == "pingpong")
                {
                    // Forward
                    for (int i = from; i <= to; i++)
                    {
                        sequence.push_back(i);
                        sequenceDurations.push_back(frameDurations[i]);
                    }
                    // Backward (excluding endpoints)
                    for (int i = to - 1; i > from; i--)
                    {
                        sequence.push_back(i);
                        sequenceDurations.push_back(frameDurations[i]);
                    }
                }
                else if (direction == "reverse")
                {
                    for (int i = to; i >= from; i--)
                    {
                        sequence.push_back(i);
                        sequenceDurations.push_back(frameDurations[i]);
                    }
                }
                else // "forward"
                {
                    for (int i = from; i <= to; i++)
                    {
                        sequence.push_back(i);
                        sequenceDurations.push_back(frameDurations[i]);
                    }
                }
                
                // Use per-frame durations if variable, otherwise uniform
                if (hasVariableDurations)
                {
                    anim->addState(tagName, sequence, sequenceDurations, true);
                }
                else
                {
                    anim->addState(tagName, sequence, uniformDuration, true);
                }
                
                LOG_INFO("AsepriteLoader: Created state '%s' (%s, frames %d-%d)", 
                        tagName.c_str(), direction.c_str(), from, to);
            }
            
            // Create states for untagged frame ranges
            int defaultCount = 0;
            int rangeStart = -1;
            
            for (int i = 0; i <= totalFrames; i++)
            {
                bool isCovered = (i < totalFrames) ? covered[i] : true; // Force end at totalFrames
                
                if (!isCovered && rangeStart == -1)
                {
                    // Start of untagged range
                    rangeStart = i;
                }
                else if (isCovered && rangeStart != -1)
                {
                    // End of untagged range
                    std::vector<int> sequence;
                    std::vector<int> sequenceDurations;
                    
                    for (int j = rangeStart; j < i; j++)
                    {
                        sequence.push_back(j);
                        sequenceDurations.push_back(frameDurations[j]);
                    }
                    
                    std::string stateName = (defaultCount == 0) ? "default" : ("default" + std::to_string(defaultCount));
                    
                    if (hasVariableDurations)
                    {
                        anim->addState(stateName, sequence, sequenceDurations, true);
                    }
                    else
                    {
                        anim->addState(stateName, sequence, uniformDuration, true);
                    }
                    
                    LOG_INFO("AsepriteLoader: Created state '%s' (frames %d-%d)", 
                            stateName.c_str(), rangeStart, i - 1);
                    
                    defaultCount++;
                    rangeStart = -1;
                }
            }
            
            // Set initial state (first state added, whether tagged or default)
            if (!covered[0])
            {
                anim->setState("default");
            }
            else
            {
                // Find first tag that covers frame 0
                for (size_t tagIdx = 0; tagIdx < frameTags.size(); tagIdx++)
                {
                    JsonValue tag = frameTags[tagIdx];
                    int from = tag["from"].asInt();
                    int to = tag["to"].asInt();
                    
                    if (from <= 0 && to >= 0)
                    {
                        std::string tagName = tag.has("name") ? tag["name"].asString() : ("tag" + std::to_string(tagIdx));
                        anim->setState(tagName);
                        break;
                    }
                }
            }

            return anim;
        }
    }

    // No tags - create simple forward animation with all frames
    if (hasVariableDurations)
    {
        std::vector<int> allFrames;
        for (int i = 0; i < totalFrames; i++)
        {
            allFrames.push_back(i);
        }
        auto anim = std::make_unique<FrameSequenceAnim>(allFrames, frameDurations, true);
        LOG_INFO("AsepriteLoader: Created simple animation with per-frame durations (frames 0-%d)", totalFrames - 1);
        return anim;
    }
    else
    {
        auto anim = FrameSequenceAnim::range(0, totalFrames - 1, uniformDuration, true);
        LOG_INFO("AsepriteLoader: Created simple animation (frames 0-%d, %dms per frame)", totalFrames - 1, uniformDuration);
        return std::make_unique<FrameSequenceAnim>(std::move(anim));
    }
}

std::unique_ptr<IAnimController> AsepriteLoader::loadAnimOnly(const std::string& jsonPath)
{
    // Read JSON file
    std::string jsonContent = readFile(jsonPath);
    if (jsonContent.empty())
    {
        return nullptr;
    }

    // Parse JSON
    JsonValue root = JsonParser::parse(jsonContent);
    if (!root.isObject() || !root.has("meta"))
    {
        return nullptr;
    }

    JsonValue meta = root["meta"];
    JsonValue frames = root["frames"];

    int totalFrames = frames.isArray() ? static_cast<int>(frames.size()) : 0;
    if (totalFrames == 0)
    {
        return nullptr;
    }

    // Collect per-frame durations
    std::vector<int> frameDurations(totalFrames, 100);  // Default 100ms
    bool hasVariableDurations = false;
    
    for (int i = 0; i < totalFrames; i++)
    {
        if (frames[i].has("duration"))
        {
            int duration = frames[i]["duration"].asInt();
            frameDurations[i] = duration;
            if (i > 0 && duration != frameDurations[0])
            {
                hasVariableDurations = true;
            }
        }
    }
    
    int uniformDuration = frameDurations[0];

    // Parse frameTags
    if (meta.has("frameTags") && meta["frameTags"].isArray())
    {
        JsonValue frameTags = meta["frameTags"];
        
        if (frameTags.size() > 0)
        {
            // Create state machine with states for tagged and untagged frame ranges
            auto anim = std::make_unique<StateMachineAnim>();
            
            // Track which frames are covered by tags
            std::vector<bool> covered(totalFrames, false);
            
            // Process all tags
            for (size_t tagIdx = 0; tagIdx < frameTags.size(); tagIdx++)
            {
                JsonValue tag = frameTags[tagIdx];
                
                int from = tag["from"].asInt();
                int to = tag["to"].asInt();
                std::string direction = tag.has("direction") ? tag["direction"].asString() : "forward";
                std::string tagName = tag.has("name") ? tag["name"].asString() : ("tag" + std::to_string(tagIdx));
                
                // Mark frames as covered
                for (int i = from; i <= to && i < totalFrames; i++)
                {
                    covered[i] = true;
                }
                
                // Create sequence based on direction
                std::vector<int> sequence;
                std::vector<int> sequenceDurations;
                
                if (direction == "pingpong")
                {
                    for (int i = from; i <= to; i++)
                    {
                        sequence.push_back(i);
                        sequenceDurations.push_back(frameDurations[i]);
                    }
                    for (int i = to - 1; i > from; i--)
                    {
                        sequence.push_back(i);
                        sequenceDurations.push_back(frameDurations[i]);
                    }
                }
                else if (direction == "reverse")
                {
                    for (int i = to; i >= from; i--)
                    {
                        sequence.push_back(i);
                        sequenceDurations.push_back(frameDurations[i]);
                    }
                }
                else
                {
                    for (int i = from; i <= to; i++)
                    {
                        sequence.push_back(i);
                        sequenceDurations.push_back(frameDurations[i]);
                    }
                }
                
                if (hasVariableDurations)
                {
                    anim->addState(tagName, sequence, sequenceDurations, true);
                }
                else
                {
                    anim->addState(tagName, sequence, uniformDuration, true);
                }
            }
            
            // Create states for untagged frame ranges
            int defaultCount = 0;
            int rangeStart = -1;
            
            for (int i = 0; i <= totalFrames; i++)
            {
                bool isCovered = (i < totalFrames) ? covered[i] : true;
                
                if (!isCovered && rangeStart == -1)
                {
                    rangeStart = i;
                }
                else if (isCovered && rangeStart != -1)
                {
                    std::vector<int> sequence;
                    std::vector<int> sequenceDurations;
                    
                    for (int j = rangeStart; j < i; j++)
                    {
                        sequence.push_back(j);
                        sequenceDurations.push_back(frameDurations[j]);
                    }
                    
                    std::string stateName = (defaultCount == 0) ? "default" : ("default" + std::to_string(defaultCount));
                    
                    if (hasVariableDurations)
                    {
                        anim->addState(stateName, sequence, sequenceDurations, true);
                    }
                    else
                    {
                        anim->addState(stateName, sequence, uniformDuration, true);
                    }
                    
                    defaultCount++;
                    rangeStart = -1;
                }
            }
            
            // Set initial state
            if (!covered[0])
            {
                anim->setState("default");
            }
            else
            {
                for (size_t tagIdx = 0; tagIdx < frameTags.size(); tagIdx++)
                {
                    JsonValue tag = frameTags[tagIdx];
                    int from = tag["from"].asInt();
                    int to = tag["to"].asInt();
                    
                    if (from <= 0 && to >= 0)
                    {
                        std::string tagName = tag.has("name") ? tag["name"].asString() : ("tag" + std::to_string(tagIdx));
                        anim->setState(tagName);
                        break;
                    }
                }
            }

            return anim;
        }
    }

    // No tags - create simple forward animation with all frames
    if (hasVariableDurations)
    {
        std::vector<int> allFrames;
        for (int i = 0; i < totalFrames; i++)
        {
            allFrames.push_back(i);
        }
        return std::make_unique<FrameSequenceAnim>(allFrames, frameDurations, true);
    }
    else
    {
        auto anim = FrameSequenceAnim::range(0, totalFrames - 1, uniformDuration, true);
        return std::make_unique<FrameSequenceAnim>(std::move(anim));
    }
}
