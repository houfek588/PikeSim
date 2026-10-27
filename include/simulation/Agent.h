#pragma once

#include "raylib.h"
#include "core/Types.h"

// --- STRUKTURA PRO JEDNOTLIVÉHO VOJÁKA ---
struct Agent {
    AgentId id{ 0 };
    Role role{ Role::PIKEMAN };
    Faction faction{ Faction::PLAYER };
    WeaponType weapon{ WeaponType::PIKE };
    Vector2 position{ 0.0f, 0.0f };
    Vector2 velocity{ 0.0f, 0.0f };
    Vector2 targetSlot{ 0.0f, 0.0f };   // Ideální pozice v mřížce formace
    float angle{ 0.0f };                // Směr pohledu/těla (v radiánech)
    float pikeAngle{ 0.0f };            // Směr píky (v radiánech)
    float swingTimer{ 0.0f };           // Časovač pro kymácení píky při chůzi
    float mass{ 1.0f };                 // Hmotnost (ovlivňuje setrvačnost)
    float maxForce{ 180.0f };           // Maximální síla pro dosažení slotu
    float pikeAngularVelocity{ 0.0f };   // Úhlová rychlost píky pro torzní soustavu
    float pikeDeflection{ 0.0f };        // Výchylka píky od klidové osy (v radiánech)
    int interlockCount{ 0 };             // Počet současně zaklesnutých nepřátelských pík
    bool isPikeInterlocked{ false };     // Příznak aktivního mechanického kontaktu pík
    float pressureForce{ 0.0f };         // Kumulativní tlačná síla z řad za ním (Push of Pikes)
    float lineDisplacement{ 0.0f };      // Posun čela vůči nominálnímu slotu
    float health{ 100.0f };              // Životy vojáka (0..100)
    bool isAlive{ true };                // Stav: živý / padlý
    bool isPikeBroken{ false };          // Zlomená píka pod tlakem nebo úderem
    bool inMelee{ false };               // Tasen poboční meč (Katzbalger)
    bool hasCuirass{ false };            // Ocelový kyrys (75% šance na odražení zranění pro přední řady)
    float attackCooldown{ 0.0f };        // Časovač útoků zblízka
};
