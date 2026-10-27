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
#include "ui/FormationCard.h"
#include <vector>

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pike Square Simulator - Tactical Formations (Milestone 3.3)");
    SetTargetFPS(60);

    std::vector<PikeSquare> playerFormations;
    playerFormations.emplace_back(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 100.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    playerFormations[0].SetFormationId(1);
    playerFormations[0].SetName("1. Praporec");
    int selectedPlayerIdx = 0;

    PikeSquare enemySquare(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 250.0f, 10, 10, 26.0f, Faction::ENEMY, PI / 2.0f);
    enemySquare.SetFormationId(2);
    enemySquare.SetName("Oponent");

    SpatialHashGrid spatialGrid(40.0f);
    EnemyAIController enemyAI(enemySquare);
    FormationCard formationCard;

    VisualMode visualMode = VisualMode::ENGRAVING;
    EngravingRenderer engravingRenderer;
    VectorRenderer vectorRenderer;
    CameraController cameraController({ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f });
    InputController inputController;

    auto registerFormation = [&](const PikeSquare& sq) {
        float sp = sq.GetSpacing() * 0.75f, pikeLen = sq.GetSpacing() * 3.2f;
        for (const auto& a : sq.GetSoldiers()) {
            if (a.isAlive) {
                spatialGrid.Insert(a.id, sq.GetFormationId(), a.faction, a.position, sp, a.pressureForce);
                if (sq.GetState() == PikeState::CHARGE && a.weapon == WeaponType::PIKE && !a.isPikeBroken) {
                    Vector2 tip = Vector2Add(a.position, Vector2Scale({ cosf(a.pikeAngle), sinf(a.pikeAngle) }, pikeLen));
                    spatialGrid.InsertPike(a.id, sq.GetFormationId(), a.faction, a.position, tip);
                }
            }
        }
    };

    while (!WindowShouldClose()) {
        float dt = fminf(GetFrameTime(), 0.1f);

        if (IsKeyPressed(KEY_T)) visualMode = (visualMode == VisualMode::ENGRAVING) ? VisualMode::VECTOR : VisualMode::ENGRAVING;
        if (IsKeyPressed(KEY_P)) enemyAI.ToggleActive();
        if (IsKeyPressed(KEY_SPACE) && inputController.IsSelected() && !playerFormations.empty()) {
            PikeState cur = playerFormations[selectedPlayerIdx].pikeState;
            playerFormations[selectedPlayerIdx].pikeState = (cur == PikeState::PORTE) ? PikeState::CHARGE : PikeState::PORTE;
        }

        cameraController.Update(dt);
        formationCard.Update(playerFormations);
        inputController.UpdateMulti(cameraController, playerFormations, selectedPlayerIdx, formationCard);
        spatialGrid.Clear();
        for (const auto& sq : playerFormations) registerFormation(sq);
        registerFormation(enemySquare);

        if (!playerFormations.empty()) enemyAI.Update(dt, playerFormations[0]);
        for (auto& sq : playerFormations) sq.Update(dt, &spatialGrid);
        enemySquare.Update(dt, &spatialGrid);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode2D(cameraController.GetCamera());
        IRenderer* r = (visualMode == VisualMode::ENGRAVING) ? static_cast<IRenderer*>(&engravingRenderer) : static_cast<IRenderer*>(&vectorRenderer);
        r->DrawBackground();
        for (const auto& sq : playerFormations) {
            r->DrawFormation(sq);
            if (sq.HasWaypoint()) r->DrawWaypoint(sq.GetWaypoint(), sq.GetFrontCenter());
        }
        r->DrawFormation(enemySquare);
        if (enemySquare.HasWaypoint() && !enemyAI.IsActive()) r->DrawWaypoint(enemySquare.GetWaypoint(), enemySquare.GetFrontCenter());
        if (inputController.IsSelected() && selectedPlayerIdx < static_cast<int>(playerFormations.size())) {
            inputController.DrawWorldOverlays(playerFormations[selectedPlayerIdx]);
        }
        EndMode2D();

        if (selectedPlayerIdx < static_cast<int>(playerFormations.size())) {
            UIManager::DrawUI(playerFormations[selectedPlayerIdx], visualMode, enemyAI.IsActive());
            UIManager::DrawFormationBar(playerFormations, selectedPlayerIdx);
            formationCard.Draw(playerFormations[selectedPlayerIdx]);
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
