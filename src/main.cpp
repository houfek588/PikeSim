#include "raylib.h"
#include "core/Types.h"
#include "formation/PikeSquare.h"
#include "rendering/EngravingRenderer.h"
#include "rendering/VectorRenderer.h"
#include "ui/UIManager.h"
#include <cmath>

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pike Square Simulator - Prototype Milestone 1");
    SetTargetFPS(60);

    PikeSquare square(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 10, 10, 26.0f);
    VisualMode visualMode = VisualMode::ENGRAVING;

    EngravingRenderer engravingRenderer;
    VectorRenderer vectorRenderer;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;

        // --- VSTUPY OD HRÁČE ---
        if (IsKeyPressed(KEY_T)) {
            visualMode = (visualMode == VisualMode::ENGRAVING) ? VisualMode::VECTOR : VisualMode::ENGRAVING;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            square.pikeState = (square.pikeState == PikeState::PORTE) ? PikeState::CHARGE : PikeState::PORTE;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            square.targetPos = GetMousePosition();
            square.hasWaypoint = true;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            square.hasWaypoint = false;
        }

        // --- AKTUALIZACE SIMULACE ---
        square.Update(dt);

        // --- VYKRESLENÍ SCÉNY ---
        BeginDrawing();

        if (visualMode == VisualMode::ENGRAVING) {
            engravingRenderer.DrawScene(square);
        } else {
            vectorRenderer.DrawScene(square);
        }

        // --- UŽIVATELSKÉ ROZHRANÍ (HUD & LEGENDA) ---
        UIManager::DrawUI(square, visualMode);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
