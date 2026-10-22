#include "ui/UIManager.h"
#include <string>

void UIManager::DrawHUD(const PikeSquare& square, VisualMode currentMode, bool aiActive) {
    DrawRectangle(10, 10, 480, 340, { 25, 25, 30, 220 });
    DrawRectangleLines(10, 10, 480, 340, { 100, 100, 110, 255 });

    // Titulek
    DrawText("PIKENYRSKY CTVEREC - 1v1 SANDBOX", 20, 20, 20, RAYWHITE);
    DrawText("Fyzikalne sjednocene veleni & O(N) Spatial Grid", 20, 44, 13, GOLD);
    DrawLine(20, 62, 460, 62, { 100, 100, 110, 255 });

    // Stav formace
    std::string modeStr = (currentMode == VisualMode::ENGRAVING) ? "Rytina (Historicky)" : "Vektorovy (Takticky)";
    DrawText(TextFormat("Vizuální styl [T]: %s", modeStr.c_str()), 20, 75, 16, LIGHTGRAY);

    std::string pikesStr = (square.pikeState == PikeState::PORTE) ? "PORTE (Na rameno / Rychly)" : "CHARGE (K utoku / Pomaly)";
    Color stateColor = (square.pikeState == PikeState::PORTE) ? SKYBLUE : ORANGE;
    DrawText(TextFormat("Stav pík [MEZERNIK]: %s", pikesStr.c_str()), 20, 100, 16, stateColor);

    // Stav oponenta AI
    const char* aiStr = aiActive ? "AKTIVNÍ (pochod k hráči)" : "STATICKÝ (sandbox)";
    Color aiColor = aiActive ? Color{ 255, 100, 100, 255 } : LIGHTGRAY;
    DrawText(TextFormat("Oponent AI [P]: %s", aiStr), 20, 125, 16, aiColor);

    // Dynamické vyobrazení tempa formace (NCO kontrola)
    float tempoFactor = square.GetNcoTempoFactor();
    Color tempoColor = GREEN;
    if (tempoFactor < 0.90f) tempoColor = ORANGE;
    if (tempoFactor < 0.50f) tempoColor = RED;
    DrawText(TextFormat("Tempo formace (vliv NCO): %.0f %%", tempoFactor * 100.0f), 20, 150, 16, tempoColor);

    // Zobrazení indexu soudržnosti (Cohesion)
    float cohesion = square.GetCohesionIndex();
    Color cohesionColor = GREEN;
    if (cohesion < 0.85f) cohesionColor = YELLOW;
    if (cohesion < 0.65f) cohesionColor = RED;

    DrawText(TextFormat("Soudržnost čtverce: %.1f %%", cohesion * 100.0f), 20, 175, 16, RAYWHITE);
    DrawRectangle(20, 197, 440, 16, { 50, 50, 60, 255 });
    DrawRectangle(20, 197, static_cast<int>(440.0f * cohesion), 16, cohesionColor);

    // Ovládací prvky a nápověda
    DrawText("OVLÁDÁNÍ:", 20, 220, 14, GOLD);
    DrawText("- [LMB]: Výběr formace | [RMB tažení]: Total War směrování", 20, 237, 13, LIGHTGRAY);
    DrawText("- [P]: Přepnutí AI oponenta (Aktivní útok / Statický terč)", 20, 253, 13, LIGHTGRAY);
    DrawText("- [W, A, S, D / MMB tažení]: Pohyb kamery | [Kolečko]: Zoom", 20, 269, 13, LIGHTGRAY);
    DrawText("- [Shift + A, D]: Okamžitý maticový obrat o 90°", 20, 285, 13, LIGHTGRAY);
    DrawText("* NCO v rozích zpomalují rotaci, pokud nestíhají tempo!", 20, 303, 12, GOLD);
    DrawText("* Linie formací se při kontaktu fyzicky odpuzují!", 20, 319, 12, SKYBLUE);
}

void UIManager::DrawLegend() {
    DrawRectangle(SCREEN_WIDTH - 250, SCREEN_HEIGHT - 150, 230, 130, { 25, 25, 30, 220 });
    DrawRectangleLines(SCREEN_WIDTH - 250, SCREEN_HEIGHT - 150, 230, 130, { 100, 100, 110, 255 });
    DrawText("LEGENDA JEDNOTEK:", SCREEN_WIDTH - 240, SCREEN_HEIGHT - 140, 13, GOLD);

    DrawCircle(SCREEN_WIDTH - 230, SCREEN_HEIGHT - 110, 7, { 180, 50, 40, 255 });
    DrawText("Velitel (Kapitán vzadu)", SCREEN_WIDTH - 212, SCREEN_HEIGHT - 117, 13, RAYWHITE);

    DrawCircle(SCREEN_WIDTH - 230, SCREEN_HEIGHT - 85, 7, { 190, 130, 40, 255 });
    DrawText("Poddůstojník (NCO na rozích)", SCREEN_WIDTH - 212, SCREEN_HEIGHT - 92, 13, RAYWHITE);

    DrawCircle(SCREEN_WIDTH - 230, SCREEN_HEIGHT - 60, 7, { 120, 110, 100, 255 });
    DrawText("Běžný pikenýr uvnitř", SCREEN_WIDTH - 212, SCREEN_HEIGHT - 67, 13, RAYWHITE);
}

void UIManager::DrawUI(const PikeSquare& square, VisualMode currentMode, bool aiActive) {
    DrawHUD(square, currentMode, aiActive);
    DrawLegend();
}
