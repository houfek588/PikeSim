#include "raylib.h"
#include "core/Types.h"
#include "core/SpatialHashGrid.h"
#include "formation/PikeSquare.h"
#include "simulation/EnemyAIController.h"
#include "rendering/EngravingRenderer.h"
#include "rendering/VectorRenderer.h"
#include "rendering/CameraController.h"
#include "ui/InputController.h"
#include "ui/UIManager.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pike Square Simulator - 1v1 Sandbox (Milestone 3.2)");
    SetTargetFPS(60);

    PikeSquare playerSquare(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 100.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    playerSquare.SetFormationId(1);
    PikeSquare enemySquare(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 250.0f, 10, 10, 26.0f, Faction::ENEMY, PI / 2.0f);
    enemySquare.SetFormationId(2);

    SpatialHashGrid spatialGrid(40.0f);
    EnemyAIController enemyAI(enemySquare);

    VisualMode visualMode = VisualMode::ENGRAVING;
    EngravingRenderer engravingRenderer;
    VectorRenderer vectorRenderer;
    CameraController cameraController({ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f });
    InputController inputController;

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.1f);

        if (IsKeyPressed(KEY_T)) visualMode = (visualMode == VisualMode::ENGRAVING) ? VisualMode::VECTOR : VisualMode::ENGRAVING;
        if (IsKeyPressed(KEY_P)) enemyAI.ToggleActive();
        if (IsKeyPressed(KEY_SPACE) && inputController.IsSelected()) {
            playerSquare.pikeState = (playerSquare.pikeState == PikeState::PORTE) ? PikeState::CHARGE : PikeState::PORTE;
        }

        cameraController.Update(dt);
        inputController.Update(cameraController, playerSquare);

        spatialGrid.Clear();
        for (const auto& a : playerSquare.GetSoldiers()) {
            spatialGrid.Insert(a.id, playerSquare.GetFormationId(), a.faction, a.position, playerSquare.GetSpacing() * 0.75f);
        }
        for (const auto& a : enemySquare.GetSoldiers()) {
            spatialGrid.Insert(a.id, enemySquare.GetFormationId(), a.faction, a.position, enemySquare.GetSpacing() * 0.75f);
        }

        enemyAI.Update(dt, playerSquare);
        playerSquare.Update(dt, &spatialGrid);
        enemySquare.Update(dt, &spatialGrid);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode2D(cameraController.GetCamera());

        IRenderer* renderer = (visualMode == VisualMode::ENGRAVING)
            ? static_cast<IRenderer*>(&engravingRenderer) : static_cast<IRenderer*>(&vectorRenderer);
        renderer->DrawBackground();
        renderer->DrawFormation(playerSquare);
        renderer->DrawFormation(enemySquare);

        if (playerSquare.HasWaypoint()) renderer->DrawWaypoint(playerSquare.GetWaypoint(), playerSquare.GetFrontCenter());
        if (enemySquare.HasWaypoint() && !enemyAI.IsActive()) renderer->DrawWaypoint(enemySquare.GetWaypoint(), enemySquare.GetFrontCenter());
        inputController.DrawWorldOverlays(playerSquare);

        EndMode2D();
        UIManager::DrawUI(playerSquare, visualMode, enemyAI.IsActive());
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
