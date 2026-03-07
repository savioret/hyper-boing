#include "main.h"
#include "weapontype.h"

const WeaponConfig& WeaponConfig::get(WeaponType type) {
    static const WeaponConfig configs[] = {
        // HARPOON: slower, 1 max shots, longer cooldown, single projectile
        {
            WeaponType::HARPOON, // type
            1,                   // maxShots
            15,                  // cooldown
            5                    // speed
        },
        // GUN: faster, 2 max shots (but 2 per shot = 4 total bullets), shorter cooldown, 2 parallel bullets
        {
            WeaponType::GUN,     // type
            3,                   // maxShots
            8,                   // cooldown
            7                    // speed
        },
        // CLAW: grappling hook that sticks to surfaces for 5 seconds
        {
            WeaponType::CLAW,    // type
            1,                   // maxShots
            15,                  // cooldown
            5                    // speed
        }
    };
    return configs[static_cast<int>(type)];
}
