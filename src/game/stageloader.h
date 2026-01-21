#pragma once

#include <string>
#include <vector>
#include <map>

class Stage;

/**
 * StageLoader utility class
 *
 * Provides functionality to load and save stage configurations from/to text files.
 * Supports a simple, human-readable format with named parameters.
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
 * # Spawn a ball at top with random X (equivalent to .atMaxY())
 * ball:
 *   start: 1
 *   y_max: true
 *
 * # Spawn a ball with random X at specific Y (equivalent to .atY(400))
 * ball:
 *   start: 1
 *   y: 400
 *   size: 2
 *
 * # Spawn a ball with random X and Y (fully random)
 * ball:
 *   start: 1
 *
 * # Spawn a ball at exact position (equivalent to .at(100, 200))
 * ball:
 *   start: 20
 *   x: 100
 *   y: 200
 *   size: 2
 *   top: 300
 *   dirX: -1
 *   dirY: 1
 *
 * # Spawn a floor (floors always use fixed position)
 * floor:
 *   start: 0
 *   x: 250
 *   y: 250
 *   floor_type: 0
 * ---
 *
 * Coordinate system (INT_MAX sentinel pattern):
 * - Omit 'x' and 'y': Both random (fully random spawn)
 * - Omit 'x' only: Random X, fixed Y
 * - Omit 'y' only: Fixed X, random Y
 * - y_max: true: Sets Y to 22 (screen top), X remains random
 * - Specify both 'x' and 'y': Fixed position
 *
 * StageObjectBuilder equivalents:
 * - .atMaxY() = y_max: true
 * - .atY(400) = y: 400 (no x specified)
 * - .atX(300) = x: 300 (no y specified)
 * - .at(100, 200) = x: 100 + y: 200
 * - (no coords) = fully random
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
    
    // Helper to parse key-value pairs
    static bool parseKeyValue(const std::string& line, std::string& key, std::string& value);
    
    // Process accumulated object data and spawn it
    static void processBallObject(Stage& stage, const std::map<std::string, std::string>& objectData);
    static void processFloorObject(Stage& stage, const std::map<std::string, std::string>& objectData);
};
