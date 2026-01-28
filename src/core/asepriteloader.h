#pragma once

#include <string>
#include <memory>
#include "spritesheet.h"
#include "animcontroller.h"

// Forward declarations
class Graph;

/**
 * AsepriteLoader - Loads Aseprite JSON format into SpriteSheet and AnimController
 *
 * Supports:
 * - Frame data (position, size, offsets)
 * - Frame tags (animation sequences)
 * - Direction types: forward, reverse, pingpong
 *
 * Usage:
 *   SpriteSheet sheet;
 *   auto anim = AsepriteLoader::load(&graph, "victory.json", sheet);
 *   
 *   // sheet now contains all frames
 *   // anim contains controller if frameTags were defined
 */
class AsepriteLoader
{
public:
    /**
     * Load Aseprite JSON and populate sprite sheet
     * 
     * @param graph Graphics context for loading texture
     * @param jsonPath Path to Aseprite JSON file
     * @param sheet SpriteSheet to populate with frames
     * @return AnimController if frameTags found, nullptr otherwise
     */
    static std::unique_ptr<IAnimController> load(
        Graph* graph,
        const std::string& jsonPath,
        SpriteSheet& sheet);

    /**
     * Load only the animation controller from JSON (assumes sheet already loaded)
     * 
     * @param jsonPath Path to Aseprite JSON file
     * @return AnimController if frameTags found, nullptr otherwise
     */
    static std::unique_ptr<IAnimController> loadAnimOnly(const std::string& jsonPath);

private:
    /**
     * Read entire file into string
     */
    static std::string readFile(const std::string& path);

    /**
     * Extract directory path from file path
     */
    static std::string getDirectory(const std::string& path);
};
