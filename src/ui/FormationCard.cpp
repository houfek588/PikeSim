#include "ui/FormationCard.h"
#include <string>

FormationCard::FormationCard()
    : m_isOpen(false)
    , m_formationIndex(0)
    , m_selectedRota(0)
{
}

Rectangle FormationCard::GetCardBounds() const {
    float w = 580.0f;
    float h = 400.0f;
    float x = (SCREEN_WIDTH - w) / 2.0f;
    float y = (SCREEN_HEIGHT - h) / 2.0f - 25.0f;
    return Rectangle{ x, y, w, h };
}

void FormationCard::Update(std::vector<PikeSquare>& playerFormations) {
    if (!m_isOpen || m_formationIndex < 0 || m_formationIndex >= static_cast<int>(playerFormations.size())) {
        m_isOpen = false;
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        Close();
        return;
    }

    PikeSquare& formation = playerFormations[m_formationIndex];
    Rectangle bounds = GetCardBounds();
    Vector2 mouse = GetMousePosition();

    // Tlačítko zavřít [X]
    Rectangle closeBtn = { bounds.x + bounds.width - 34, bounds.y + 10, 24, 24 };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, closeBtn)) {
        Close();
        return;
    }

    // Ovládání hloubky řad (počet vojáků v hloubce)
    float depthY = bounds.y + 115.0f;
    Rectangle btnDepthMinus = { bounds.x + 130.0f, depthY, 26.0f, 22.0f };
    Rectangle btnDepthPlus  = { bounds.x + 162.0f, depthY, 26.0f, 22.0f };
    Rectangle presets[4] = {
        { bounds.x + 295.0f, depthY, 55.0f, 22.0f },
        { bounds.x + 355.0f, depthY, 55.0f, 22.0f },
        { bounds.x + 415.0f, depthY, 55.0f, 22.0f },
        { bounds.x + 475.0f, depthY, 60.0f, 22.0f }
    };
    int presetVals[4] = { 5, 6, 8, 10 };

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, btnDepthMinus)) {
            formation.SetFormationDepth(formation.GetRows() - 1);
        } else if (CheckCollisionPointRec(mouse, btnDepthPlus)) {
            formation.SetFormationDepth(formation.GetRows() + 1);
        } else {
            for (int p = 0; p < 4; ++p) {
                if (CheckCollisionPointRec(mouse, presets[p])) {
                    formation.SetFormationDepth(presetVals[p]);
                    break;
                }
            }
        }
    }

    // Výběr roty kliknutím na sloupec
    int rotaCount = static_cast<int>(formation.roty.size());
    if (m_selectedRota >= rotaCount) m_selectedRota = rotaCount - 1;

    float gridStartX = bounds.x + 25.0f;
    float gridStartY = bounds.y + 165.0f;
    float colWidth = 46.0f;
    float colHeight = 130.0f;
    float colGap = 6.0f;

    for (int i = 0; i < rotaCount; ++i) {
        Rectangle colRect = { gridStartX + i * (colWidth + colGap), gridStartY, colWidth, colHeight };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, colRect)) {
            m_selectedRota = i;
        }
    }

    // Tlačítko [<- Vlevo]
    Rectangle btnLeft = { bounds.x + 25.0f, bounds.y + 320.0f, 110.0f, 32.0f };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, btnLeft)) {
        if (m_selectedRota > 0) {
            formation.SwapRoty(m_selectedRota, m_selectedRota - 1);
            m_selectedRota--;
        }
    }

    // Tlačítko [Vpravo ->]
    Rectangle btnRight = { bounds.x + 145.0f, bounds.y + 320.0f, 110.0f, 32.0f };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, btnRight)) {
        if (m_selectedRota < rotaCount - 1) {
            formation.SwapRoty(m_selectedRota, m_selectedRota + 1);
            m_selectedRota++;
        }
    }

    // Tlačítko [ODDĚLIT ROTU]
    Rectangle btnDetach = { bounds.x + 275.0f, bounds.y + 320.0f, 150.0f, 32.0f };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, btnDetach)) {
        if (rotaCount > 1 && m_selectedRota >= 0 && m_selectedRota < rotaCount) {
            PikeSquare detached = formation.DetachRota(m_selectedRota);
            playerFormations.push_back(detached);
            rotaCount = static_cast<int>(formation.roty.size());
            if (m_selectedRota >= rotaCount) m_selectedRota = rotaCount - 1;
        }
    }

    // Tlačítko [HOTOVO]
    Rectangle btnDone = { bounds.x + 440.0f, bounds.y + 320.0f, 115.0f, 32.0f };
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, btnDone)) {
        Close();
    }
}

void FormationCard::Draw(const PikeSquare& formation) const {
    if (!m_isOpen) return;

    Rectangle bounds = GetCardBounds();
    Vector2 mouse = GetMousePosition();

    // Podklad modálního okna
    DrawRectangleRec(bounds, { 20, 22, 28, 250 });
    DrawRectangleLinesEx(bounds, 2.0f, { 190, 160, 90, 255 });

    // Záhlaví
    DrawRectangle(static_cast<int>(bounds.x), static_cast<int>(bounds.y), static_cast<int>(bounds.width), 42, { 30, 34, 44, 255 });
    DrawLine(static_cast<int>(bounds.x), static_cast<int>(bounds.y + 42), static_cast<int>(bounds.x + bounds.width), static_cast<int>(bounds.y + 42), { 190, 160, 90, 255 });
    DrawText(TextFormat("KARTA FORMACE: %s", formation.GetName().c_str()), static_cast<int>(bounds.x + 20), static_cast<int>(bounds.y + 12), 18, GOLD);

    // Křížek pro zavření
    Rectangle closeBtn = { bounds.x + bounds.width - 34, bounds.y + 10, 24, 24 };
    bool hoverClose = CheckCollisionPointRec(mouse, closeBtn);
    DrawRectangleRec(closeBtn, hoverClose ? Color{ 180, 50, 40, 255 } : Color{ 50, 50, 60, 255 });
    DrawText("X", static_cast<int>(closeBtn.x + 7), static_cast<int>(closeBtn.y + 4), 16, RAYWHITE);

    // 1. STATISTICKÝ PANEL
    float statsY = bounds.y + 55.0f;
    DrawText("STAV MUŽSTVA:", static_cast<int>(bounds.x + 25), static_cast<int>(statsY), 14, GOLD);

    int pikemen = formation.GetPikemenCount();
    int musketeers = formation.GetMusketeerCount();
    int halberds = formation.GetHalberdierCount();
    int totalSoldiers = static_cast<int>(formation.GetSoldiers().size());

    DrawText(TextFormat("Živých: %d  |  Píky: %d  Muškety: %d  Halaparty: %d", totalSoldiers, pikemen, musketeers, halberds),
             static_cast<int>(bounds.x + 25), static_cast<int>(statsY + 22), 14, RAYWHITE);

    // Morálka
    float barWidth = 240.0f;
    DrawText("Morálka:", static_cast<int>(bounds.x + 25), static_cast<int>(statsY + 46), 13, LIGHTGRAY);
    DrawRectangle(static_cast<int>(bounds.x + 95), static_cast<int>(statsY + 46), static_cast<int>(barWidth), 14, { 45, 45, 55, 255 });
    DrawRectangle(static_cast<int>(bounds.x + 95), static_cast<int>(statsY + 46), static_cast<int>(barWidth * formation.GetMorale()), 14, GREEN);

    // Stamina
    DrawText("Stamina:", static_cast<int>(bounds.x + 25), static_cast<int>(statsY + 66), 13, LIGHTGRAY);
    DrawRectangle(static_cast<int>(bounds.x + 95), static_cast<int>(statsY + 66), static_cast<int>(barWidth), 14, { 45, 45, 55, 255 });
    DrawRectangle(static_cast<int>(bounds.x + 95), static_cast<int>(statsY + 66), static_cast<int>(barWidth * formation.GetStamina()), 14, SKYBLUE);

    // 2. OVLÁDÁNÍ HLOUBKY FORMACE
    float depthY = bounds.y + 115.0f;
    DrawText(TextFormat("Hloubka: %d řad", formation.GetRows()), static_cast<int>(bounds.x + 25), static_cast<int>(depthY + 4), 13, GOLD);

    Rectangle btnMinus = { bounds.x + 130.0f, depthY, 26.0f, 22.0f };
    Rectangle btnPlus  = { bounds.x + 162.0f, depthY, 26.0f, 22.0f };
    bool hovMinus = CheckCollisionPointRec(mouse, btnMinus);
    bool hovPlus  = CheckCollisionPointRec(mouse, btnPlus);

    DrawRectangleRec(btnMinus, hovMinus ? Color{ 55, 65, 85, 255 } : Color{ 35, 40, 52, 255 });
    DrawRectangleLinesEx(btnMinus, 1.0f, { 100, 110, 130, 255 });
    DrawText("-", static_cast<int>(btnMinus.x + 9), static_cast<int>(btnMinus.y + 2), 16, RAYWHITE);

    DrawRectangleRec(btnPlus, hovPlus ? Color{ 55, 65, 85, 255 } : Color{ 35, 40, 52, 255 });
    DrawRectangleLinesEx(btnPlus, 1.0f, { 100, 110, 130, 255 });
    DrawText("+", static_cast<int>(btnPlus.x + 8), static_cast<int>(btnPlus.y + 2), 16, RAYWHITE);

    DrawText("Předvolby:", static_cast<int>(bounds.x + 225), static_cast<int>(depthY + 4), 12, LIGHTGRAY);

    Rectangle presets[4] = {
        { bounds.x + 295.0f, depthY, 55.0f, 22.0f },
        { bounds.x + 355.0f, depthY, 55.0f, 22.0f },
        { bounds.x + 415.0f, depthY, 55.0f, 22.0f },
        { bounds.x + 475.0f, depthY, 60.0f, 22.0f }
    };
    const char* presetLabels[4] = { "5 řad", "6 řad", "8 řad", "10 řad" };
    int presetVals[4] = { 5, 6, 8, 10 };

    for (int p = 0; p < 4; ++p) {
        bool isAct = (formation.GetRows() == presetVals[p]);
        bool hov = CheckCollisionPointRec(mouse, presets[p]);
        DrawRectangleRec(presets[p], isAct ? Color{ 50, 75, 110, 255 } : (hov ? Color{ 40, 50, 65, 255 } : Color{ 30, 35, 45, 255 }));
        DrawRectangleLinesEx(presets[p], 1.0f, isAct ? GOLD : Color{ 80, 90, 110, 255 });
        DrawText(presetLabels[p], static_cast<int>(presets[p].x + 8), static_cast<int>(presets[p].y + 4), 12, isAct ? GOLD : RAYWHITE);
    }

    // 3. VIZUÁLNÍ SCHÉMA ZÁSTUPŮ (ROTY)
    DrawText("ROTY A ZÁSTUPY (Kliknutím vyberte zástup k přesunu či oddělení):", static_cast<int>(bounds.x + 25), static_cast<int>(bounds.y + 145), 13, GOLD);

    int rotaCount = static_cast<int>(formation.roty.size());
    float gridStartX = bounds.x + 25.0f;
    float gridStartY = bounds.y + 165.0f;
    float colWidth = 46.0f;
    float colHeight = 140.0f;
    float colGap = 6.0f;

    for (int i = 0; i < rotaCount; ++i) {
        Rectangle colRect = { gridStartX + i * (colWidth + colGap), gridStartY, colWidth, colHeight };
        bool isSel = (i == m_selectedRota);
        bool hoverCol = CheckCollisionPointRec(mouse, colRect);

        Color colBg = isSel ? Color{ 45, 55, 75, 255 } : (hoverCol ? Color{ 35, 40, 50, 255 } : Color{ 28, 30, 38, 255 });
        DrawRectangleRec(colRect, colBg);
        DrawRectangleLinesEx(colRect, isSel ? 2.0f : 1.0f, isSel ? GOLD : Color{ 80, 85, 100, 255 });

        // Popisek roty
        DrawText(TextFormat("R%d", i + 1), static_cast<int>(colRect.x + 12), static_cast<int>(colRect.y + 6), 14, isSel ? GOLD : RAYWHITE);

        // Typ zbraně
        const char* wBadge = "[P]";
        if (formation.roty[i].weaponType == WeaponType::MUSKET) wBadge = "[M]";
        if (formation.roty[i].weaponType == WeaponType::HALBERD) wBadge = "[H]";
        DrawText(wBadge, static_cast<int>(colRect.x + 13), static_cast<int>(colRect.y + 24), 13, SKYBLUE);

        // Znázornění vojáků v hloubce zástupu
        int depth = static_cast<int>(formation.roty[i].soldierIndices.size());
        for (int r = 0; r < depth && r < 10; ++r) {
            float dotY = colRect.y + 45.0f + r * 8.5f;
            DrawCircle(static_cast<int>(colRect.x + colWidth / 2.0f), static_cast<int>(dotY), 3.0f, isSel ? GOLD : Color{ 140, 160, 180, 200 });
        }
    }

    // 3. OVLÁDACÍ TLAČÍTKA
    Rectangle btnLeft = { bounds.x + 25.0f, bounds.y + 320.0f, 110.0f, 32.0f };
    bool hoverLeft = CheckCollisionPointRec(mouse, btnLeft);
    DrawRectangleRec(btnLeft, hoverLeft ? Color{ 50, 60, 80, 255 } : Color{ 35, 40, 55, 255 });
    DrawRectangleLinesEx(btnLeft, 1.0f, { 100, 110, 130, 255 });
    DrawText("<- Vlevo", static_cast<int>(btnLeft.x + 22), static_cast<int>(btnLeft.y + 8), 14, RAYWHITE);

    Rectangle btnRight = { bounds.x + 145.0f, bounds.y + 320.0f, 110.0f, 32.0f };
    bool hoverRight = CheckCollisionPointRec(mouse, btnRight);
    DrawRectangleRec(btnRight, hoverRight ? Color{ 50, 60, 80, 255 } : Color{ 35, 40, 55, 255 });
    DrawRectangleLinesEx(btnRight, 1.0f, { 100, 110, 130, 255 });
    DrawText("Vpravo ->", static_cast<int>(btnRight.x + 20), static_cast<int>(btnRight.y + 8), 14, RAYWHITE);

    Rectangle btnDetach = { bounds.x + 275.0f, bounds.y + 320.0f, 150.0f, 32.0f };
    bool canDetach = (rotaCount > 1);
    bool hoverDetach = canDetach && CheckCollisionPointRec(mouse, btnDetach);
    DrawRectangleRec(btnDetach, hoverDetach ? Color{ 180, 110, 40, 255 } : (canDetach ? Color{ 110, 70, 30, 255 } : Color{ 40, 40, 45, 255 }));
    DrawRectangleLinesEx(btnDetach, 1.0f, canDetach ? GOLD : Color{ 70, 70, 80, 255 });
    DrawText("ODDĚLIT ROTU", static_cast<int>(btnDetach.x + 18), static_cast<int>(btnDetach.y + 8), 13, canDetach ? RAYWHITE : GRAY);

    Rectangle btnDone = { bounds.x + 440.0f, bounds.y + 320.0f, 115.0f, 32.0f };
    bool hoverDone = CheckCollisionPointRec(mouse, btnDone);
    DrawRectangleRec(btnDone, hoverDone ? Color{ 50, 120, 60, 255 } : Color{ 35, 80, 45, 255 });
    DrawRectangleLinesEx(btnDone, 1.0f, GREEN);
    DrawText("HOTOVO", static_cast<int>(btnDone.x + 28), static_cast<int>(btnDone.y + 8), 14, RAYWHITE);

    DrawText("* Tip: Vyberte rotu a klikněte na 'ODDĚLIT ROTU' pro vytvoření nového oddílu.", static_cast<int>(bounds.x + 25), static_cast<int>(bounds.y + 368), 12, GOLD);
}
