#pragma once

#include <string>
#include <memory>
#include "spritesheet.h"
#include "animcontroller.h"

// Forward declarations
class Graph;

/**
 * @brief Loads Aseprite JSON sprite sheet data into SpriteSheet and AnimController objects.
 *
 * Parses the JSON format exported by Aseprite ("Array" or "Hash" frame layout) and
 * populates a SpriteSheet with per-frame UV rectangles and trim offsets. Animation
 * controllers are built from the frame timing and optional @c frameTags metadata.
 *
 * Supported Aseprite export features:
 * - Frame data: position, size, spriteSourceSize offsets, sourceSize
 * - Frame tags: named animation states with direction (forward / reverse / pingpong)
 *   and repeat count (0 = infinite loop, 1 = play once, N = repeat N times)
 * - Variable per-frame durations
 *
 * @par Example
 * @code
 * SpriteSheet sheet;
 * // Auto-detect animation type
 * auto anim = AsepriteLoader::load(&graph, "victory.json", sheet);
 *
 * // Typed specialisations (no cast needed)
 * auto sm  = AsepriteLoader::loadAsStateMachine(&graph, "player.json", sheet);
 * auto seq = AsepriteLoader::loadAsSequence(&graph, "walk.json", sheet);
 *
 * // Reuse the same JSON with a different PNG
 * auto anim2 = AsepriteLoader::load(&graph, "player.json", sheet2, "assets/player_red.png");
 * @endcode
 */
class AsepriteLoader
{
public:
    /**
     * @brief Load a sprite sheet and auto-detect the animation type.
     *
     * Parses @p jsonPath, loads the PNG into @p sheet, and returns an
     * IAnimController whose concrete type is determined by the JSON content:
     * - StateMachineAnim  — when @c frameTags are present.
     * - FrameSequenceAnim — when no tags are present (all frames, uniform or variable duration).
     *
     * @param graph      Renderer used to create the SDL texture.
     * @param jsonPath   Path to the Aseprite-exported @c .json file.
     * @param sheet      Output sprite sheet populated with frame data.
     * @param imagePath  Optional override for the PNG path. When empty (default) the
     *                   path embedded in the JSON @c meta.image field is used, resolved
     *                   relative to @p jsonPath. Pass a non-empty value to reuse the
     *                   same JSON with a different PNG.
     * @return Owning pointer to the animation controller, or @c nullptr on failure.
     */
    static std::unique_ptr<IAnimController> load(
        Graph* graph,
        const std::string& jsonPath,
        SpriteSheet& sheet,
        const std::string& imagePath = "");

    /**
     * @brief Load only the animation controller from a JSON file (no sprite sheet).
     *
     * Useful when the SpriteSheet has already been loaded separately or the same
     * sheet is shared across multiple animations.
     * Auto-detects the animation type exactly like load().
     *
     * @param jsonPath  Path to the Aseprite-exported @c .json file.
     * @return Owning pointer to the animation controller, or @c nullptr on failure.
     */
    static std::unique_ptr<IAnimController> loadAnimOnly(const std::string& jsonPath);

    /**
     * @brief Load a sprite sheet and return a StateMachineAnim.
     *
     * Each Aseprite @c frameTag becomes a named state. If no tags are defined,
     * all frames are placed in a single @c "default" state.
     * The initial active state is set to the tag covering frame 0 (or @c "default").
     *
     * @param graph      Renderer used to create the SDL texture.
     * @param jsonPath   Path to the Aseprite-exported @c .json file.
     * @param sheet      Output sprite sheet populated with frame data.
     * @param imagePath  Optional PNG path override (see load() for details).
     * @return Owning pointer to the StateMachineAnim, or @c nullptr on failure.
     */
    static std::unique_ptr<StateMachineAnim> loadAsStateMachine(
        Graph* graph,
        const std::string& jsonPath,
        SpriteSheet& sheet,
        const std::string& imagePath = "");

    /**
     * @brief Load only a StateMachineAnim from a JSON file (no sprite sheet).
     *
     * Useful when the SpriteSheet is managed elsewhere.
     * State building rules are identical to loadAsStateMachine(Graph*, ...).
     *
     * @param jsonPath  Path to the Aseprite-exported @c .json file.
     * @return Owning pointer to the StateMachineAnim, or @c nullptr on failure.
     */
    static std::unique_ptr<StateMachineAnim> loadAsStateMachine(const std::string& jsonPath);

    /**
     * @brief Load a sprite sheet and return a FrameSequenceAnim.
     *
     * All frames are loaded into a single sequential animation regardless of any
     * @c frameTags in the JSON. Useful for simple one-shot or looping animations
     * that do not need named states.
     *
     * @param graph      Renderer used to create the SDL texture.
     * @param jsonPath   Path to the Aseprite-exported @c .json file.
     * @param sheet      Output sprite sheet populated with frame data.
     * @param imagePath  Optional PNG path override (see load() for details).
     * @return Owning pointer to the FrameSequenceAnim, or @c nullptr on failure.
     */
    static std::unique_ptr<FrameSequenceAnim> loadAsSequence(
        Graph* graph,
        const std::string& jsonPath,
        SpriteSheet& sheet,
        const std::string& imagePath = "");

    /**
     * @brief Load only a FrameSequenceAnim from a JSON file (no sprite sheet).
     *
     * Useful when the SpriteSheet is managed elsewhere.
     * Frame sequencing rules are identical to loadAsSequence(Graph*, ...).
     *
     * @param jsonPath  Path to the Aseprite-exported @c .json file.
     * @return Owning pointer to the FrameSequenceAnim, or @c nullptr on failure.
     */
    static std::unique_ptr<FrameSequenceAnim> loadAsSequence(const std::string& jsonPath);
};
