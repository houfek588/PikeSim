#pragma once

#include <cstdint>

// --- KONSTANTY OBRAZOVKY A OKNA ---
constexpr int SCREEN_WIDTH = 1200;
constexpr int SCREEN_HEIGHT = 800;

// --- ROLE JEDNOTEK VE FORMACI ---
enum class Role {
    PIKEMAN,    // Řadový pikenýr (tvoří plné jádro formace)
    NCO,        // Poddůstojník (hlídá rohy vně formace a určuje tempo)
    LEADER      // Kapitán / Velitel (řídí jednotku zezadu)
};

// --- STAV ZBRANÍ (TAKTIKA) ---
enum class PikeState {
    PORTE,      // Píky nahoře (rychlejší přesun)
    CHARGE      // Píky skloněné dopředu (pomalý bojový pochod)
};

// --- TYPY ZBRANÍ PRO ROTY ---
enum class WeaponType {
    PIKE,       // Píka (hloubka 10 vojáků v rotě)
    MUSKET,     // Mušketa (hloubka 6 vojáků v rotě)
    HALBERD     // Halaparta (hloubka 6 nebo 10 vojáků v rotě)
};

// --- REŽIM VIZUALIZACE ---
enum class VisualMode {
    ENGRAVING,  // Styl staré papírové rytiny
    VECTOR      // Minimalistický taktický vektorový režim
};

// --- STRANY NA BOJIŠTI (FRAKCE) ---
enum class Faction {
    PLAYER,     // Modří (hráč)
    ENEMY       // Červení (protivník)
};

// --- IDENTIFIKÁTORY ---
using AgentId = int;
using FormationId = int;
