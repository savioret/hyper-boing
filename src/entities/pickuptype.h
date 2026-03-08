#pragma once

/**
 * PickupType enum
 *
 * Types of collectible pickups in the game.
 * Kept in a separate header so it can be included without
 * pulling in the full Pickup entity and its dependencies.
 */
enum class PickupType : int
{
    GUN = 0,           // Switch weapon to GunShot
    DOUBLE_SHOOT = 1,  // Harpoon allows 2 shots (until weapon change or death)
    EXTRA_TIME = 2,    // Add 20 seconds to timer
    TIME_FREEZE = 3,   // Freeze balls for 10 seconds
    EXTRA_LIFE = 4,    // Add 1 life
    SHIELD = 5,        // Survive 1 hit
    CLAW = 6           // Placeholder (not implemented)
};

/// Maximum number of per-size death pickups stored on a single ball/hexa.
static constexpr int MAX_DEATH_PICKUPS = 4;

/**
 * DeathPickupEntry
 *
 * Associates a pickup type with the ball/hexa size at which it is spawned.
 */
struct DeathPickupEntry
{
    int        size = 0;
    PickupType type = PickupType::GUN;
};
