#pragma once

#include "formation/PikeSquare.h"

// --- STAV TAKTICKÉHO CHOVÁNÍ OPONENTA ---
enum class EnemyAIState {
    IDLE,       // Statický (sandbox cíl)
    ADVANCE,    // Pochod vstříc hráči v postoji PORTE
    ENGAGE      // Kontaktní boj v postoji CHARGE (skloněné píky)
};

// --- KONTROLER JEDNODUCHÉ TAKTICKÉ AI OPONENTA ---
class EnemyAIController {
public:
    explicit EnemyAIController(PikeSquare& controlledFormation);

    // Hlavní rozhodovací tick AI
    void Update(float dt, const PikeSquare& playerFormation);

    void SetActive(bool active) { m_isActive = active; }
    void ToggleActive() { m_isActive = !m_isActive; }
    bool IsActive() const { return m_isActive; }
    EnemyAIState GetState() const { return m_state; }

private:
    PikeSquare& m_formation;
    EnemyAIState m_state{ EnemyAIState::ADVANCE };
    bool m_isActive{ true };
    float m_repathTimer{ 0.0f };
};
