#pragma once

#include <unordered_set>
#include <string>

/**
 * OnceHelper - Tracks fired events to avoid boolean flag pollution
 *
 * This utility class helps eliminate scattered boolean flags throughout the codebase
 * by tracking string-based keys in a set. Perfect for "do once" patterns like playing
 * sounds on specific triggers, showing tutorials, or achieving milestones.
 *
 * Usage:
 *   OnceHelper helper;
 *   if (helper.once("event_key")) {
 *       // Executes only the first time
 *   }
 *
 * Two-Level Context:
 *   - Game-level (AppData): Persists across all stages, cleared on new game
 *   - Stage-level (Scene): Resets every stage, cleared in Scene::init()
 */
class OnceHelper
{
private:
    std::unordered_set<std::string> fired;

public:
    /**
     * Check if key has been fired before
     *
     * Returns true only the FIRST time called with a given key.
     * Subsequent calls with the same key return false.
     *
     * @param key Unique identifier for this event
     * @return true if first time, false if already fired
     */
    bool once(const std::string& key);

    /**
     * Reset a specific key (allows it to fire again)
     *
     * After reset, calling once() with this key will return true again.
     *
     * @param key The key to reset
     */
    void reset(const std::string& key);

    /**
     * Clear all fired keys
     *
     * Useful for resetting stage-level helpers when transitioning between stages.
     * Called automatically in Scene::init() for stage-level helper.
     */
    void clear();

    /**
     * Check if key has been fired (read-only)
     *
     * Unlike once(), this doesn't modify state or mark the key as fired.
     *
     * @param key The key to check
     * @return true if key has been fired, false otherwise
     */
    bool hasFired(const std::string& key) const;
};
