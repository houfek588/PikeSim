#pragma once

#include "raylib.h"
#include "formation/PikeSquare.h"
#include "rendering/CameraController.h"

class FormationCard;

// --- OVLADAČ RTS VSTUPŮ (TOTAL WAR STYL VÝBĚRU A POVELŮ) ---
class InputController {
public:
    InputController();

    // Původní aktualizace vstupů pro 1 formaci (zpětná kompatibilita)
    void Update(const CameraController& camera, PikeSquare& formation);

    // Rozšířená aktualizace pro více přátelských formací a kartu formace
    void UpdateMulti(CameraController& camera, std::vector<PikeSquare>& formations, int& selectedIndex, FormationCard& card);

    // Vykreslení vizuálních pomůcek ve světovém prostoru (indikátor výběru, ghost silueta cíle)
    void DrawWorldOverlays(const PikeSquare& formation) const;

    // Stav výběru jednotky
    bool IsSelected() const { return m_isSelected; }
    void SetSelected(bool sel) { m_isSelected = sel; }

    // Stav tažení RMB
    bool IsRmbDragging() const { return m_isRmbDragging; }
    Vector2 GetDragStart() const { return m_rmbDragStartWorld; }
    Vector2 GetDragCurrent() const { return m_rmbDragCurrentWorld; }
    float GetDragHeading() const { return m_dragHeading; }

    // Pomocný výpočet směru z drag vektoru (pro jednotkové testy)
    static float CalculateHeadingFromVector(Vector2 start, Vector2 current);

    // Pomocná detekce kolize myši s formací
    static bool IsMouseOverFormation(Vector2 worldMouse, const PikeSquare& formation);

    // Detekce dvojkliku (pro jednotkové testy a UI)
    bool CheckDoubleClick(Vector2 mousePos);

private:
    bool m_isSelected{ true }; // Defaultně vybrána při spuštění pro okamžitou hratelnost
    bool m_isRmbDragging{ false };
    Vector2 m_rmbDragStartWorld{ 0.0f, 0.0f };
    Vector2 m_rmbDragCurrentWorld{ 0.0f, 0.0f };
    float m_dragHeading{ 0.0f };

    float m_lastLmbClickTime{ -10.0f };
    Vector2 m_lastLmbClickPos{ 0.0f, 0.0f };

    void DrawSelectionIndicator(const PikeSquare& formation) const;
    void DrawGhostSilhouette(const PikeSquare& formation) const;
};
