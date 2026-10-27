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
    if (square.IsPushing()) {
        DrawText(TextFormat("* PUSH OF PIKES: Tlak %.0f N | Padlí: %d | Zlomeno pík: %d", square.GetAverageFrontPressure(), square.GetCasualtiesCount(), square.GetBrokenPikesCount()), 20, 319, 12, RED);
    } else {
        DrawText(TextFormat("* Mužstvo: %d živých | %d padlých | %d zlomených pík", square.GetLivingCount(), square.GetCasualtiesCount(), square.GetBrokenPikesCount()), 20, 319, 12, SKYBLUE);
    }
}

void UIManager::DrawFormationBar(const std::vector<PikeSquare>& formations, int selectedIndex) {
    if (formations.empty()) return;

    float barY = SCREEN_HEIGHT - 75.0f;
    float startX = 20.0f;
    float cardWidth = 140.0f;
    float cardHeight = 62.0f;
    float gap = 10.0f;

    Vector2 mouse = GetMousePosition();

    for (size_t i = 0; i < formations.size(); ++i) {
        Rectangle cardRect = { startX + i * (cardWidth + gap), barY, cardWidth, cardHeight };
        bool isSel = (static_cast<int>(i) == selectedIndex);
        bool isHover = CheckCollisionPointRec(mouse, cardRect);

        Color bg = isSel ? Color{ 40, 48, 65, 235 } : (isHover ? Color{ 32, 36, 46, 230 } : Color{ 24, 26, 33, 220 });
        DrawRectangleRec(cardRect, bg);
        DrawRectangleLinesEx(cardRect, isSel ? 2.0f : 1.0f, isSel ? GOLD : Color{ 80, 85, 100, 200 });

        // Název jednotky
        DrawText(formations[i].GetName().c_str(), static_cast<int>(cardRect.x + 8), static_cast<int>(cardRect.y + 6), 13, isSel ? GOLD : RAYWHITE);

        // Počet mužů a stav
        int count = static_cast<int>(formations[i].GetSoldiers().size());
        DrawText(TextFormat("%d mužů", count), static_cast<int>(cardRect.x + 8), static_cast<int>(cardRect.y + 24), 12, LIGHTGRAY);

        const char* stateStr = (formations[i].pikeState == PikeState::PORTE) ? "PORTE" : "CHARGE";
        Color stateColor = (formations[i].pikeState == PikeState::PORTE) ? SKYBLUE : ORANGE;
        DrawText(stateStr, static_cast<int>(cardRect.x + cardWidth - 55), static_cast<int>(cardRect.y + 24), 11, stateColor);

        // Morálka (úzký proužek dole)
        DrawRectangle(static_cast<int>(cardRect.x + 8), static_cast<int>(cardRect.y + 44), static_cast<int>(cardWidth - 16), 6, { 45, 45, 55, 255 });
        DrawRectangle(static_cast<int>(cardRect.x + 8), static_cast<int>(cardRect.y + 44), static_cast<int>((cardWidth - 16) * formations[i].GetMorale()), 6, GREEN);
    }
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
