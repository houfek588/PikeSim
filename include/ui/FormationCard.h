#pragma once

#include "raylib.h"
#include "formation/PikeSquare.h"
#include <vector>

// --- KARTA FORMACE (INSPEKTOR PRASETE / EDITOR ZÁSTUPŮ ROT) ---
class FormationCard {
public:
    FormationCard();

    void Open(int formationIndex) {
        m_isOpen = true;
        m_formationIndex = formationIndex;
        m_selectedRota = 0;
    }
    void Close() { m_isOpen = false; }
    void Toggle(int formationIndex) {
        if (m_isOpen && m_formationIndex == formationIndex) {
            Close();
        } else {
            Open(formationIndex);
        }
    }

    bool IsOpen() const { return m_isOpen; }
    int GetFormationIndex() const { return m_formationIndex; }
    int GetSelectedRota() const { return m_selectedRota; }
    void SetSelectedRota(int idx) { m_selectedRota = idx; }

    // Aktualizace vstupů (kliknutí na roty, tlačítka prohození a oddělení)
    void Update(std::vector<PikeSquare>& playerFormations);

    // Vykreslení modálního okna karty formace
    void Draw(const PikeSquare& formation) const;

private:
    bool m_isOpen{ false };
    int m_formationIndex{ 0 };
    int m_selectedRota{ 0 };

    Rectangle GetCardBounds() const;
};
