#pragma once

#include <string>
#include <vector>
#include <map>

class Stage;

/**
 * StageLoader utility class
 *
 * Provides functionality to load and save stage configurations from/to text files.
 * Supports a YAML-style indentation-based format with time blocks.
 *
 * File format example:
 * ---
 * stage_id: 1
 * background: fondo1.png
 * music: stage1.ogg
 * time_limit: 100
 * player1_x: 250
 * player2_x: 350
 *
 * # Objects grouped by spawn time (in seconds)
 * at 0:
 *   floor: x=550, y=50, type=0
 *   floor: x=250, y=250, type=0
 *
 * at 1:
 *   ball: y_max=true
 *
 * at 10:
 *   ball: y_max=true, size=2
 *   /weapon gun 1
 * ---
 *
 * Object syntax:
 * - type: key=value, key=value  (comma-separated)
 * - type: key=value key=value   (space-separated)
 * - Boolean flags without =value default to true (e.g., y_max)
 *
 * Ball properties:
 * - x, y: Position (omit for random)
 * - y_max: Sets Y to screen top (22px), X random
 * - size: Ball size (0=largest, 3=smallest)
 * - top: Maximum jump height (0=auto)
 * - dirX: Horizontal direction (-1=left, 1=right)
 * - dirY: Vertical direction (-1=up, 1=down)
 * - type: Ball type/color
 *
 * Floor properties:
 * - x, y: Position (required)
 * - type: Floor type (0=horizontal, 1=vertical)
 *
 * Actions:
 * - Lines starting with / are console commands executed at spawn time
 * - Example: /weapon gun 1
 */
class StageLoader
{
public:
    /**
     * Load stage configuration from a file
     * @param stage Stage object to populate
     * @param filename Path to stage file
     * @return true if successful, false otherwise
     */
    static bool load(Stage& stage, const std::string& filename);

    /**
     * Save stage configuration to a file
     * @param stage Stage object to serialize
     * @param filename Path to output file
     * @return true if successful, false otherwise
     */
    static bool save(const Stage& stage, const std::string& filename);

private:
    // Helper to trim whitespace
    static std::string trim(const std::string& str);

    // Measure indentation level (number of leading spaces/tabs)
    static int getIndentLevel(const std::string& line);

    // Parse "at <time>:" line, returns time value or -1 on failure
    static float parseTimeBlock(const std::string& line);

    // Parse stage-level key-value pairs (stage_id, background, etc.)
    static bool parseStageProperty(Stage& stage, const std::string& key, const std::string& value);

    // Parse object line: "ball: x=100, y=200, size=2"
    static bool parseObjectLine(Stage& stage, float currentTime, const std::string& line);

    // Parse action line: "/weapon gun 1"
    static bool parseActionLine(Stage& stage, float currentTime, const std::string& line);

    // Parse key=value pairs from object definition
    static std::map<std::string, std::string> parseParams(const std::string& paramString);

    // Process accumulated object data and spawn it
    static void processBallObject(Stage& stage, float time, const std::map<std::string, std::string>& params);
    static void processFloorObject(Stage& stage, float time, const std::map<std::string, std::string>& params);
    static void processActionObject(Stage& stage, float time, const std::string& command);
};
