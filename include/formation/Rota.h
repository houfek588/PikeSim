#pragma once

#include "core/Types.h"
#include <vector>

// --- STRUKTURA PRO JEDEN ZÁSTUP (ROTU) FORMACE ---
struct Rota {
    int id{ 0 };
    WeaponType weaponType{ WeaponType::PIKE };
    int depth{ 10 };                    // Hloubka zástupu (10 pro piky, 6 pro muškety)
    std::vector<int> soldierIndices;    // Indexy vojáků v poli soldiers[] mateřského praporce od čela do hloubky
};
