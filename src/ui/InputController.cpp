#include "ui/InputController.h"
#include "ui/FormationCard.h"
#include "core/MathUtils.h"
#include "raymath.h"
#include <cmath>

InputController::InputController() = default;

float InputController::CalculateHeadingFromVector(Vector2 start, Vector2 current) {
    float dx = current.x - start.x;
    float dy = current.y - start.y;
    return atan2f(dy, dx);
}

bool InputController::CheckDoubleClick(Vector2 mousePos) {
    float now = static_cast<float>(GetTime());
    bool isDouble = (now - m_lastLmbClickTime < 0.35f) && (Vector2Distance(mousePos, m_lastLmbClickPos) < 15.0f);
    m_lastLmbClickTime = now;
    m_lastLmbClickPos = mousePos;
    return isDouble;
}

bool InputController::IsMouseOverFormation(Vector2 worldMouse, const PikeSquare& formation) {
    // 1. Rychlý test vzdálenosti od geometrického středu
    float boundRadius = (formation.GetCols() * formation.GetSpacing()) * 0.75f;
    if (Vector2Distance(worldMouse, formation.GetPosition()) <= boundRadius) {
        return true;
    }

    // 2. Přesný test blízkosti k jakémukoliv vojákovi
    float threshold = formation.GetSpacing() * 1.5f;
    for (const auto& soldier : formation.GetSoldiers()) {
        if (Vector2Distance(worldMouse, soldier.position) < threshold) {
            return true;
        }
    }
    return false;
}

void InputController::Update(const CameraController& camera, PikeSquare& formation) {
    Vector2 mouseScreen = GetMousePosition();
    Vector2 mouseWorld = camera.ScreenToWorld(mouseScreen);

    // --- 1. VÝBĚR FORMACE LEVÝM TLAČÍTKEM MYŠI (LMB) ---
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (IsMouseOverFormation(mouseWorld, formation)) {
            m_isSelected = true;
        } else {
            m_isSelected = false;
        }
    }

    // --- 2. ZADÁVÁNÍ POVELŮ PRAVÝM TLAČÍTKEM MYŠI (RMB) ---
    if (m_isSelected) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            m_isRmbDragging = true;
            m_rmbDragStartWorld = mouseWorld;
            m_rmbDragCurrentWorld = mouseWorld;
            m_dragHeading = formation.GetAngle();
        }

        if (m_isRmbDragging && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            m_rmbDragCurrentWorld = mouseWorld;
            float dragDistance = Vector2Distance(m_rmbDragStartWorld, m_rmbDragCurrentWorld);
            if (dragDistance > 15.0f) {
                m_dragHeading = CalculateHeadingFromVector(m_rmbDragStartWorld, m_rmbDragCurrentWorld);
            }
        }

        if (m_isRmbDragging && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            float dragDistance = Vector2Distance(m_rmbDragStartWorld, m_rmbDragCurrentWorld);
            if (dragDistance > 15.0f) {
                formation.SetOrder(m_rmbDragStartWorld, m_dragHeading, true);
            } else {
                formation.SetOrder(m_rmbDragStartWorld, 0.0f, false);
            }
            m_isRmbDragging = false;
        }
    } else {
        m_isRmbDragging = false;
    }
}

void InputController::UpdateMulti(CameraController& camera, std::vector<PikeSquare>& formations, int& selectedIndex, FormationCard& card) {
    if (formations.empty()) return;
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(formations.size())) {
        selectedIndex = 0;
    }

    Vector2 mouseScreen = GetMousePosition();
    Vector2 mouseWorld = camera.ScreenToWorld(mouseScreen);

    // Pokud je otevřena karta formace, ignorujeme kliknutí do herního světa pod ní
    if (card.IsOpen()) return;

    // --- 1. KONTROLA KLIKNUTÍ DO SPODNÍ LIŠTY FORMACÍ ---
    float barY = SCREEN_HEIGHT - 75.0f;
    float cardWidth = 140.0f;
    float cardHeight = 62.0f;
    float gap = 10.0f;
    float startX = 20.0f;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        bool isDoubleClick = CheckDoubleClick(mouseScreen);
        bool clickedOnBar = false;

        for (size_t i = 0; i < formations.size(); ++i) {
            Rectangle cardRect = { startX + i * (cardWidth + gap), barY, cardWidth, cardHeight };
            if (CheckCollisionPointRec(mouseScreen, cardRect)) {
                selectedIndex = static_cast<int>(i);
                m_isSelected = true;
                clickedOnBar = true;
                if (isDoubleClick) {
                    camera.CenterOn(formations[i].GetPosition());
                    card.Open(selectedIndex);
                }
                break;
            }
        }

        if (!clickedOnBar) {
            int clickedFormationIdx = -1;
            for (size_t i = 0; i < formations.size(); ++i) {
                if (IsMouseOverFormation(mouseWorld, formations[i])) {
                    clickedFormationIdx = static_cast<int>(i);
                    break;
                }
            }

            if (clickedFormationIdx >= 0) {
                selectedIndex = clickedFormationIdx;
                m_isSelected = true;
                if (isDoubleClick) {
                    card.Open(selectedIndex);
                }
            } else {
                m_isSelected = false;
            }
        }
    }

    // --- 2. ZADÁVÁNÍ POVELŮ PRAVÝM TLAČÍTKEM MYŠI (RMB) VYBRANÉ FORMACI ---
    if (m_isSelected && selectedIndex >= 0 && selectedIndex < static_cast<int>(formations.size())) {
        PikeSquare& activeFormation = formations[selectedIndex];

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            m_isRmbDragging = true;
            m_rmbDragStartWorld = mouseWorld;
            m_rmbDragCurrentWorld = mouseWorld;
            m_dragHeading = activeFormation.GetAngle();
        }

        if (m_isRmbDragging && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            m_rmbDragCurrentWorld = mouseWorld;
            float dragDistance = Vector2Distance(m_rmbDragStartWorld, m_rmbDragCurrentWorld);
            if (dragDistance > 15.0f) {
                m_dragHeading = CalculateHeadingFromVector(m_rmbDragStartWorld, m_rmbDragCurrentWorld);
            }
        }

        if (m_isRmbDragging && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            float dragDistance = Vector2Distance(m_rmbDragStartWorld, m_rmbDragCurrentWorld);
            if (dragDistance > 15.0f) {
                activeFormation.SetOrder(m_rmbDragStartWorld, m_dragHeading, true);
            } else {
                activeFormation.SetOrder(m_rmbDragStartWorld, 0.0f, false);
            }
            m_isRmbDragging = false;
        }
    } else {
        m_isRmbDragging = false;
    }
}

void InputController::DrawWorldOverlays(const PikeSquare& formation) const {
    if (m_isSelected) {
        DrawSelectionIndicator(formation);
    }

    if (m_isRmbDragging) {
        float dragDistance = Vector2Distance(m_rmbDragStartWorld, m_rmbDragCurrentWorld);
        if (dragDistance > 15.0f) {
            DrawGhostSilhouette(formation);
        }
    }
}

void InputController::DrawSelectionIndicator(const PikeSquare& formation) const {
    // Decentní kruh výběru kolem těžiště formace
    Vector2 center = formation.GetPosition();
    float radius = (formation.GetCols() * formation.GetSpacing()) * 0.65f;

    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius, Color{ 46, 204, 113, 160 });
    DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), radius + 1.0f, Color{ 46, 204, 113, 90 });

    // Zvýraznění čela jednotky malým zeleným bodem
    Vector2 frontCenter = formation.GetFrontCenter();
    DrawCircle(static_cast<int>(frontCenter.x), static_cast<int>(frontCenter.y), 4.5f, Color{ 46, 204, 113, 220 });
}

void InputController::DrawGhostSilhouette(const PikeSquare& formation) const {
    // 1. Vektorová šipka směru tažení
    DrawLineEx(m_rmbDragStartWorld, m_rmbDragCurrentWorld, 2.0f, Color{ 52, 152, 219, 180 });

    // 2. Náhled budoucí pozice a natočení všech vojáků formace
    int cols = formation.GetCols();
    int rows = formation.GetRows();
    float spacing = formation.GetSpacing();
    float colOffset = (cols - 1) / 2.0f;
    float rowOffset = (rows - 1) / 2.0f;
    float rotAngle = m_dragHeading + PI / 2.0f;

    Vector2 pikeDir = { cosf(m_dragHeading), sinf(m_dragHeading) };

    // Vykreslení mřížky pikenýrů
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float localX = (c - colOffset) * spacing;
            float localY = (r - rowOffset) * spacing;

            Vector2 rotatedOffset = MathUtils::RotateVector({ localX, localY }, rotAngle);
            Vector2 ghostPos = Vector2Add(m_rmbDragStartWorld, rotatedOffset);

            // Poloprůhledné kolečko vojáka
            DrawCircle(static_cast<int>(ghostPos.x), static_cast<int>(ghostPos.y), 3.5f, Color{ 46, 204, 113, 110 });

            // Skloněný dřík píky
            Vector2 pikeEnd = Vector2Add(ghostPos, Vector2Scale(pikeDir, 26.0f));
            DrawLineEx(ghostPos, pikeEnd, 1.2f, Color{ 230, 230, 230, 95 });
        }
    }

    // Vykreslení čelní linie pro jasnou vizuální orientaci
    Vector2 frontLeft = Vector2Add(m_rmbDragStartWorld, MathUtils::RotateVector({ -colOffset * spacing, -rowOffset * spacing }, rotAngle));
    Vector2 frontRight = Vector2Add(m_rmbDragStartWorld, MathUtils::RotateVector({ colOffset * spacing, -rowOffset * spacing }, rotAngle));
    DrawLineEx(frontLeft, frontRight, 2.5f, Color{ 46, 204, 113, 200 });
}
