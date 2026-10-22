#pragma once

#include "raylib.h"
#include "core/Types.h"

// --- STRUKTURA PRO JEDNOTLIVÉHO VOJÁKA ---
struct Agent {
    AgentId id{ 0 };
    Role role{ Role::PIKEMAN };
    Faction faction{ Faction::PLAYER };
    Vector2 position{ 0.0f, 0.0f };
    Vector2 velocity{ 0.0f, 0.0f };
    Vector2 targetSlot{ 0.0f, 0.0f };   // Ideální pozice v mřížce formace
    float angle{ 0.0f };                // Směr pohledu/těla (v radiánech)
    float pikeAngle{ 0.0f };            // Směr píky (v radiánech)
    float swingTimer{ 0.0f };           // Časovač pro kymácení píky při chůzi
    float mass{ 1.0f };                 // Hmotnost (ovlivňuje setrvačnost)
    float maxForce{ 180.0f };           // Maximální síla pro dosažení slotu
};
