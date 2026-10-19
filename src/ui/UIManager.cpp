#include "ui/UIManager.h"
#include <string>

void UIManager::DrawHUD(const PikeSquare& square, VisualMode currentMode) {
    DrawRectangle(10, 10, 480, 320, { 25, 25, 30, 220 });
    DrawRectangleLines(10, 10, 480, 320, { 100, 100, 110, 255 });

    // Titulek
    DrawText("PIKENYRSKY CTVEREC - PROTOTYP V1.2", 20, 20, 20, RAYWHITE);
    DrawText("Fyzikalne sjednocene veleni", 20, 44, 13, GOLD);
    DrawLine(20, 62, 460, 62, { 100, 100, 110, 255 });

    // Stav formace
    std::string modeStr = (currentMode == VisualMode::ENGRAVING) ? "Rytina (Historicky)" : "Vektorovy (Takticky)";
    DrawText(TextFormat("Vizuální styl [T]: %s", modeStr.c_str()), 20, 75, 16, LIGHTGRAY);

    std::string pikesStr = (square.pikeState == PikeState::PORTE) ? "PORTE (Na rameno / Rychly)" : "CHARGE (K utoku / Pomaly)";
    Color stateColor = (square.pikeState == PikeState::PORTE) ? SKYBLUE : ORANGE;
    DrawText(TextFormat("Stav pík [MEZERNIK]: %s", pikesStr.c_str()), 20, 100, 16, stateColor);

    // Dynamické vyobrazení tempa formace (NCO kontrola)
    float tempoFactor = square.GetNcoTempoFactor();
    Color tempoColor = GREEN;
    if (tempoFactor < 0.90f) tempoColor = ORANGE;
    if (tempoFactor < 0.50f) tempoColor = RED;
    DrawText(TextFormat("Tempo formace (vliv NCO): %.0f %%", tempoFactor * 100.0f), 20, 125, 16, tempoColor);

    // Zobrazení indexu soudržnosti (Cohesion)
    float cohesion = square.GetCohesionIndex();
    Color cohesionColor = GREEN;
    if (cohesion < 0.85f) cohesionColor = YELLOW;
    if (cohesion < 0.65f) cohesionColor = RED;

    DrawText(TextFormat("Soudržnost čtverce: %.1f %%", cohesion * 100.0f), 20, 150, 16, RAYWHITE);
    DrawRectangle(20, 175, 440, 18, { 50, 50, 60, 255 });
    DrawRectangle(20, 175, static_cast<int>(440.0f * cohesion), 18, cohesionColor);

    // Ovládací prvky a nápověda
    DrawText("OVLÁDÁNÍ:", 20, 200, 14, GOLD);
    DrawText("- [W, S]: Pohyb vpřed/vzad", 20, 218, 13, LIGHTGRAY);
    DrawText("- [A, D]: Kolo (Rotace kolem př. LEVÉHO / PRAVÉHO rohu)", 20, 234, 13, LIGHTGRAY);
    DrawText("- [Shift + A, D]: Obrat na místě o 90° (Změna čela čtverce)", 20, 250, 13, LIGHTGRAY);
    DrawText("- [Klik myši L]: Waypoint (Pochod za středem 1. řady)", 20, 266, 13, LIGHTGRAY);
    DrawText("- [Klik myši R]: Zrušit waypoint", 20, 282, 13, LIGHTGRAY);
    DrawText("* NCO v rozích zpomalují rotaci, pokud nestíhají vnější poloměr!", 20, 301, 12, GOLD);
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

void UIManager::DrawUI(const PikeSquare& square, VisualMode currentMode) {
    DrawHUD(square, currentMode);
    DrawLegend();
}
