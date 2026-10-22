#pragma once

#include "raylib.h"
#include "core/Types.h"
#include "formation/PikeSquare.h"

// --- SPRÁVCE UŽIVATELSKÉHO ROZHRANÍ (HUD & LEGENDA) ---
class UIManager {
public:
    // Vykreslí kompletní uživatelské rozhraní (HUD panel i legendu)
    static void DrawUI(const PikeSquare& square, VisualMode currentMode, bool aiActive = true);

    // Vykreslí hlavní informační panel vlevo nahoře
    static void DrawHUD(const PikeSquare& square, VisualMode currentMode, bool aiActive = true);

    // Vykreslí legendu jednotek vpravo dole
    static void DrawLegend();
};
