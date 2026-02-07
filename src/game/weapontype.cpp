#include "main.h"
#include "weapontype.h"

const WeaponConfig& WeaponConfig::get(WeaponType type) {
    static const WeaponConfig configs[] = {
        // HARPOON: slower, 2 max shots, longer cooldown, single projectile
        {
            WeaponType::HARPOON, // type
            2,                   // maxShots
            15,                  // cooldown
            5                    // speed
        },
        // GUN: faster, 2 max shots (but 2 per shot = 4 total bullets), shorter cooldown, 2 parallel bullets
        {
            WeaponType::GUN,     // type
            5,                   // maxShots
            8,                   // cooldown
            7                    // speed
        }
    };
    return configs[static_cast<int>(type)];
}
