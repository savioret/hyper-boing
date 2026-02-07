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
 *   // Typed specializations (no casting needed):
 *   auto sm = AsepriteLoader::loadAsStateMachine(&graph, "player.json", sheet);
 *   auto seq = AsepriteLoader::loadAsSequence(&graph, "walk.json", sheet);
 */
class AsepriteLoader
{
public:
    /**
     * Load Aseprite JSON and populate sprite sheet.
     * Auto-detects animation type: StateMachineAnim if frameTags found,
     * FrameSequenceAnim otherwise.
     */
    static std::unique_ptr<IAnimController> load(
        Graph* graph,
        const std::string& jsonPath,
        SpriteSheet& sheet);

    /**
     * Load only animation controller from JSON (no sprite sheet loading).
     * Auto-detects animation type: StateMachineAnim if frameTags found,
     * FrameSequenceAnim otherwise.
     */
    static std::unique_ptr<IAnimController> loadAnimOnly(const std::string& jsonPath);

    /**
     * Load as StateMachineAnim with sprite sheet.
     * If no frameTags, all frames go into a "default" state.
     */
    static std::unique_ptr<StateMachineAnim> loadAsStateMachine(
        Graph* graph,
        const std::string& jsonPath,
        SpriteSheet& sheet);

    /**
     * Load animation only as StateMachineAnim (no sprite sheet loading).
     * If no frameTags, all frames go into a "default" state.
     */
    static std::unique_ptr<StateMachineAnim> loadAsStateMachine(const std::string& jsonPath);

    /**
     * Load as FrameSequenceAnim with sprite sheet.
     * Ignores frameTags - loads all frames as a single sequential animation.
     */
    static std::unique_ptr<FrameSequenceAnim> loadAsSequence(
        Graph* graph,
        const std::string& jsonPath,
        SpriteSheet& sheet);

    /**
     * Load animation only as FrameSequenceAnim (no sprite sheet loading).
     * Ignores frameTags - loads all frames as a single sequential animation.
     */
    static std::unique_ptr<FrameSequenceAnim> loadAsSequence(const std::string& jsonPath);
};
