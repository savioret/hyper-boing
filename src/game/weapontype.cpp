#include "main.h"
#include "weapontype.h"

const WeaponConfig& WeaponConfig::get(WeaponType type) {
    static const WeaponConfig configs[] = {
        // HARPOON: slower, 2 max shots, longer cooldown, single projectile
        {
            WeaponType::HARPOON, // type
            2,                   // maxShots
            15,                  // cooldown
            5,                   // speed
            1,                   // projectileCount
            0                    // horizontalSpacing
        },
        // DOUBLE HARPOON: moderate speed, 2 max shots, shorter cooldown, 2 parallel projectiles
        {
            WeaponType::HARPOON2, // type
            2,                   // maxShots
            7 ,                  // cooldown
            8,                   // speed
            2,                   // projectileCount
            8                    // horizontalSpacing
        },
        // GUN: faster, 2 max shots (but 2 per shot = 4 total bullets), shorter cooldown, 2 parallel bullets
        {
            WeaponType::GUN,     // type
            5,                   // maxShots
            8,                   // cooldown
            7,                   // speed
            1,                   // projectileCount
            8                    // horizontalSpacing
        }
    };
    return configs[static_cast<int>(type)];
}
