#pragma once

/**
 * WeaponType enum
 *
 * Defines the available weapon types in the game.
 */
enum class WeaponType {
    HARPOON = 0,  // Current chain weapon,
    HARPOON2 = 1,  // Current chain weapon
    GUN = 2       // New bullet weapon
};

/**
 * WeaponConfig struct
 *
 * Configuration data for each weapon type.
 * Contains gameplay parameters like speed, cooldown, and projectile behavior.
 */
struct WeaponConfig {
    WeaponType type;
    int maxShots;          // Max simultaneous projectiles per player
    int cooldown;          // Frames between shots
    int speed;             // Pixels per frame upward
    int projectileCount;   // Number of projectiles per shot
    int projectileSpacing; // Horizontal spacing between projectiles

    /**
     * Get the configuration for a specific weapon type
     * @param type The weapon type
     * @return Reference to the weapon configuration
     */
    static const WeaponConfig& get(WeaponType type);
};
