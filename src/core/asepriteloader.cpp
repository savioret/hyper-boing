#include "asepriteloader.h"
#include "jsonparser.h"
#include "graph.h"
#include "logger.h"
#include <fstream>
#include <sstream>

// ============================================================================
// Internal helpers (anonymous namespace)
// ============================================================================
namespace {

struct FrameTimingData
{
    int totalFrames = 0;
    std::vector<int> frameDurations;
    bool hasVariableDurations = false;
    int uniformDuration = 100;
};

std::string readFile(const std::string& path)
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

std::string getDirectory(const std::string& path)
{
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        return path.substr(0, lastSlash + 1);
    }
    return "";
}

// Helper to get frame count regardless of format (array or hash)
size_t getFrameCount(const JsonValue& frames)
{
    return frames.size();
}

// Helper to get frame data by index, regardless of format
JsonValue getFrameAt(const JsonValue& frames, size_t index)
{
    if (frames.isArray())
    {
        return frames[index];
    }
    else if (frames.isObject())
    {
        // For object/hash format, get keys and access by index
        std::vector<std::string> keys = frames.getKeys();
        if (index < keys.size())
        {
            return frames[keys[index]];
        }
    }
    return JsonValue(); // Return null value if index out of range
}

bool parseJson(const std::string& jsonPath, JsonValue& root)
{
    std::string content = readFile(jsonPath);
    if (content.empty())
    {
        LOG_ERROR("AsepriteLoader: Failed to read JSON file");
        return false;
    }

    root = JsonParser::parse(content);
    if (!root.isObject())
    {
        LOG_ERROR("AsepriteLoader: Invalid JSON format");
        return false;
    }

    if (!root.has("meta"))
    {
        LOG_ERROR("AsepriteLoader: No 'meta' section found");
        return false;
    }

    if (!root.has("frames"))
    {
        LOG_ERROR("AsepriteLoader: No 'frames' found");
        return false;
    }

    JsonValue frames = root["frames"];
    if (!frames.isArray() && !frames.isObject())
    {
        LOG_ERROR("AsepriteLoader: 'frames' must be an array or object");
        return false;
    }

    return true;
}

bool loadSpriteSheet(Graph* graph, const JsonValue& root, const std::string& jsonPath,
                     SpriteSheet& sheet, const std::string& overrideImagePath = "")
{
    std::string imagePath;
    if (!overrideImagePath.empty())
    {
        imagePath = overrideImagePath;
    }
    else
    {
        JsonValue meta = root["meta"];
        if (!meta.has("image"))
        {
            LOG_ERROR("AsepriteLoader: No 'image' field in meta");
            return false;
        }
        std::string dir = getDirectory(jsonPath);
        imagePath = dir + meta["image"].asString();
    }

    if (!sheet.init(graph, imagePath))
    {
        LOG_ERROR("AsepriteLoader: Failed to load texture: %s", imagePath.c_str());
        return false;
    }

    JsonValue frames = root["frames"];
    size_t frameCount = getFrameCount(frames);

    for (size_t i = 0; i < frameCount; i++)
    {
        JsonValue frameData = getFrameAt(frames, i);

        if (!frameData.has("frame") || !frameData.has("spriteSourceSize"))
        {
            LOG_WARNING("AsepriteLoader: Frame %zu missing required data", i);
            continue;
        }

        JsonValue frame = frameData["frame"];
        int x = frame["x"].asInt();
        int y = frame["y"].asInt();
        int w = frame["w"].asInt();
        int h = frame["h"].asInt();

        JsonValue spriteSource = frameData["spriteSourceSize"];
        int xOff = spriteSource["x"].asInt();
        int yOff = spriteSource["y"].asInt();

        int srcW = 0, srcH = 0;
        if (frameData.has("sourceSize"))
        {
            JsonValue sourceSize = frameData["sourceSize"];
            srcW = sourceSize["w"].asInt();
            srcH = sourceSize["h"].asInt();
        }

        sheet.addFrame(x, y, w, h, xOff, yOff, srcW, srcH);
    }

    //LOG_INFO("AsepriteLoader: Loaded %zu frames from %s", frameCount, imagePath.c_str());
    return true;
}

FrameTimingData extractTimingData(const JsonValue& frames)
{
    FrameTimingData data;
    data.totalFrames = static_cast<int>(getFrameCount(frames));
    data.frameDurations.resize(data.totalFrames, 100);

    for (int i = 0; i < data.totalFrames; i++)
    {
        JsonValue frameData = getFrameAt(frames, i);
        if (frameData.has("duration"))
        {
            int duration = frameData["duration"].asInt();
            data.frameDurations[i] = duration;
            if (i > 0 && duration != data.frameDurations[0])
            {
                data.hasVariableDurations = true;
            }
        }
    }

    data.uniformDuration = data.frameDurations.empty() ? 100 : data.frameDurations[0];

    if (!data.hasVariableDurations)
    {
        //LOG_INFO("AsepriteLoader: Using uniform duration of %dms for all frames", data.uniformDuration);
    }
    else
    {
        //LOG_INFO("AsepriteLoader: Using per-frame durations (variable)");
    }

    return data;
}

void buildDirectionSequence(
    const std::string& direction,
    int from, int to,
    const std::vector<int>& frameDurations,
    std::vector<int>& sequence,
    std::vector<int>& sequenceDurations)
{
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
    else // "forward"
    {
        for (int i = from; i <= to; i++)
        {
            sequence.push_back(i);
            sequenceDurations.push_back(frameDurations[i]);
        }
    }
}

void addAnimState(
    StateMachineAnim& anim,
    const std::string& name,
    const std::vector<int>& sequence,
    const std::vector<int>& sequenceDurations,
    const FrameTimingData& timing,
    int loopCount)
{
    if (timing.hasVariableDurations)
    {
        anim.addState(name, sequence, sequenceDurations, loopCount);
    }
    else
    {
        anim.addState(name, sequence, timing.uniformDuration, loopCount);
    }
}

bool hasFrameTags(const JsonValue& meta)
{
    return meta.has("frameTags") && meta["frameTags"].isArray() && meta["frameTags"].size() > 0;
}

// Build StateMachineAnim from frameTags (tagged states + untagged "default" ranges)
std::unique_ptr<StateMachineAnim> buildStateMachineFromTags(
    const JsonValue& meta,
    const FrameTimingData& timing)
{
    auto anim = std::make_unique<StateMachineAnim>();
    JsonValue frameTags = meta["frameTags"];
    std::vector<bool> covered(timing.totalFrames, false);

    // Process all tags
    for (size_t tagIdx = 0; tagIdx < frameTags.size(); tagIdx++)
    {
        JsonValue tag = frameTags[tagIdx];

        int from = tag["from"].asInt();
        int to = tag["to"].asInt();
        std::string direction = tag.has("direction") ? tag["direction"].asString() : "forward";
        std::string tagName = tag.has("name") ? tag["name"].asString() : ("tag" + std::to_string(tagIdx));

        // Parse "repeat" field: if not present or "0", infinite loop (0)
        // If "1", play once (1), if "2"+, repeat N times
        int loopCount = 0;  // Default: infinite loop
        if (tag.has("repeat"))
        {
            std::string repeatStr = tag["repeat"].asString();
            int repeatVal = std::atoi(repeatStr.c_str());
            loopCount = repeatVal;  // 0 = infinite, 1 = play once, 2+ = repeat N times
        }

        for (int i = from; i <= to && i < timing.totalFrames; i++)
        {
            covered[i] = true;
        }

        std::vector<int> sequence;
        std::vector<int> sequenceDurations;
        buildDirectionSequence(direction, from, to, timing.frameDurations, sequence, sequenceDurations);
        addAnimState(*anim, tagName, sequence, sequenceDurations, timing, loopCount);

        //const char* loopDesc = (loopCount == 0) ? "infinite" : (loopCount == 1) ? "once" : "repeat";
        //LOG_INFO("AsepriteLoader: Created state '%s' (%s, frames %d-%d, %s)",
       //         tagName.c_str(), direction.c_str(), from, to, loopDesc);
    }

    // Create states for untagged frame ranges
    int defaultCount = 0;
    int rangeStart = -1;

    for (int i = 0; i <= timing.totalFrames; i++)
    {
        bool isCovered = (i < timing.totalFrames) ? covered[i] : true;

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
                sequenceDurations.push_back(timing.frameDurations[j]);
            }

            std::string stateName = (defaultCount == 0) ? "default" : ("default" + std::to_string(defaultCount));
            addAnimState(*anim, stateName, sequence, sequenceDurations, timing, 0);  // 0 = infinite loop

            //LOG_INFO("AsepriteLoader: Created state '%s' (frames %d-%d, infinite)",
            //        stateName.c_str(), rangeStart, i - 1);

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

// Build StateMachineAnim with all frames in a single "default" state
std::unique_ptr<StateMachineAnim> buildDefaultStateMachine(const FrameTimingData& timing)
{
    auto anim = std::make_unique<StateMachineAnim>();

    std::vector<int> allFrames;
    for (int i = 0; i < timing.totalFrames; i++)
    {
        allFrames.push_back(i);
    }

    addAnimState(*anim, "default", allFrames, timing.frameDurations, timing, 0);  // 0 = infinite loop
    anim->setState("default");

    //LOG_INFO("AsepriteLoader: Created StateMachineAnim with 'default' state (frames 0-%d, infinite)", timing.totalFrames - 1);
    return anim;
}

// Build FrameSequenceAnim with all frames (ignoring any tags)
std::unique_ptr<FrameSequenceAnim> buildSequence(const FrameTimingData& timing)
{
    if (timing.hasVariableDurations)
    {
        std::vector<int> allFrames;
        for (int i = 0; i < timing.totalFrames; i++)
        {
            allFrames.push_back(i);
        }
        //LOG_INFO("AsepriteLoader: Created FrameSequenceAnim with per-frame durations (frames 0-%d, infinite)", timing.totalFrames - 1);
        return std::make_unique<FrameSequenceAnim>(allFrames, timing.frameDurations, 0);  // 0 = infinite loop
    }
    else
    {
        //LOG_INFO("AsepriteLoader: Created FrameSequenceAnim (frames 0-%d, %dms per frame, infinite)", timing.totalFrames - 1, timing.uniformDuration);
        auto anim = FrameSequenceAnim::range(0, timing.totalFrames - 1, timing.uniformDuration, 0);  // 0 = infinite loop
        return std::make_unique<FrameSequenceAnim>(std::move(anim));
    }
}

// High-level builders that compose timing + animation creation from parsed JSON

std::unique_ptr<IAnimController> buildAnimFromJson(const JsonValue& root)
{
    JsonValue meta = root["meta"];
    JsonValue frames = root["frames"];
    FrameTimingData timing = extractTimingData(frames);

    if (timing.totalFrames == 0)
        return nullptr;

    if (hasFrameTags(meta))
        return buildStateMachineFromTags(meta, timing);

    return buildSequence(timing);
}

std::unique_ptr<StateMachineAnim> buildStateMachineFromJson(const JsonValue& root)
{
    JsonValue meta = root["meta"];
    JsonValue frames = root["frames"];
    FrameTimingData timing = extractTimingData(frames);

    if (timing.totalFrames == 0)
        return nullptr;

    if (hasFrameTags(meta))
        return buildStateMachineFromTags(meta, timing);

    return buildDefaultStateMachine(timing);
}

std::unique_ptr<FrameSequenceAnim> buildSequenceFromJson(const JsonValue& root)
{
    JsonValue frames = root["frames"];
    FrameTimingData timing = extractTimingData(frames);

    if (timing.totalFrames == 0)
        return nullptr;

    return buildSequence(timing);
}

} // anonymous namespace

// ============================================================================
// Public API
// ============================================================================

std::unique_ptr<IAnimController> AsepriteLoader::load(
    Graph* graph,
    const std::string& jsonPath,
    SpriteSheet& sheet,
    const std::string& imagePath)
{
    JsonValue root;
    if (!parseJson(jsonPath, root))
        return nullptr;

    if (!loadSpriteSheet(graph, root, jsonPath, sheet, imagePath))
        return nullptr;

    return buildAnimFromJson(root);
}

std::unique_ptr<IAnimController> AsepriteLoader::loadAnimOnly(const std::string& jsonPath)
{
    JsonValue root;
    if (!parseJson(jsonPath, root))
        return nullptr;

    return buildAnimFromJson(root);
}

std::unique_ptr<StateMachineAnim> AsepriteLoader::loadAsStateMachine(
    Graph* graph,
    const std::string& jsonPath,
    SpriteSheet& sheet,
    const std::string& imagePath)
{
    JsonValue root;
    if (!parseJson(jsonPath, root))
        return nullptr;

    if (!loadSpriteSheet(graph, root, jsonPath, sheet, imagePath))
        return nullptr;

    return buildStateMachineFromJson(root);
}

std::unique_ptr<StateMachineAnim> AsepriteLoader::loadAsStateMachine(const std::string& jsonPath)
{
    JsonValue root;
    if (!parseJson(jsonPath, root))
        return nullptr;

    return buildStateMachineFromJson(root);
}

std::unique_ptr<FrameSequenceAnim> AsepriteLoader::loadAsSequence(
    Graph* graph,
    const std::string& jsonPath,
    SpriteSheet& sheet,
    const std::string& imagePath)
{
    JsonValue root;
    if (!parseJson(jsonPath, root))
        return nullptr;

    if (!loadSpriteSheet(graph, root, jsonPath, sheet, imagePath))
        return nullptr;

    return buildSequenceFromJson(root);
}

std::unique_ptr<FrameSequenceAnim> AsepriteLoader::loadAsSequence(const std::string& jsonPath)
{
    JsonValue root;
    if (!parseJson(jsonPath, root))
        return nullptr;

    return buildSequenceFromJson(root);
}
