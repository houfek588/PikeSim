#pragma once

#include "raylib.h"
#include "core/Types.h"
#include "formation/PikeSquare.h"

// --- ABSTRAKTNÍ ROZHRANÍ PRO VYKRESLOVÁNÍ ---
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Vykreslení celé scény včetně pozadí, formace a waypointu
    virtual void DrawScene(const PikeSquare& square) = 0;

    // Vykreslení pozadí scény (mřížka, barva podkladu)
    virtual void DrawBackground() = 0;

    // Vykreslení vojáků a formace
    virtual void DrawFormation(const PikeSquare& square) = 0;

    // Vykreslení jednotlivého agenta (tělo, přilba, pero, zbraň, stín)
    virtual void DrawAgent(const Agent& agent, float spacing, PikeState state) = 0;

    // Vykreslení cílového waypointu
    virtual void DrawWaypoint(Vector2 targetPos, Vector2 frontCenter) = 0;

    // Alternativní volání kompatibilní s Render()
    virtual void Render(const PikeSquare& square) { DrawScene(square); }
};
