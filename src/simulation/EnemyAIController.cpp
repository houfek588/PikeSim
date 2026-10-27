#include "simulation/EnemyAIController.h"
#include "core/MathUtils.h"
#include <cmath>

EnemyAIController::EnemyAIController(PikeSquare& controlledFormation)
    : m_formation(controlledFormation)
    , m_state(EnemyAIState::IDLE)
    , m_isActive(false)
    , m_repathTimer(0.0f)
{
}

void EnemyAIController::Update(float dt, const PikeSquare& playerFormation) {
    if (!m_isActive) {
        if (m_state != EnemyAIState::IDLE) {
            m_state = EnemyAIState::IDLE;
            m_formation.ClearWaypoint();
        }
        return;
    }

    Vector2 enemyPos = m_formation.GetPosition();
    Vector2 playerPos = playerFormation.GetPosition();
    Vector2 toPlayer = Vector2Subtract(playerPos, enemyPos);
    float dist = Vector2Length(toPlayer);

    float desiredHeading = atan2f(toPlayer.y, toPlayer.x);

    m_repathTimer += dt;

    if (dist > 250.0f) {
        m_state = EnemyAIState::ADVANCE;
        m_formation.pikeState = PikeState::PORTE;

        if (m_repathTimer >= 0.5f || !m_formation.HasWaypoint()) {
            m_repathTimer = 0.0f;
            Vector2 targetDest = Vector2Add(enemyPos, Vector2Scale(MathUtils::SafeNormalize(toPlayer), dist - 60.0f));
            m_formation.SetOrder(targetDest, desiredHeading, true);
        }
    } else {
        m_state = EnemyAIState::ENGAGE;
        m_formation.pikeState = PikeState::CHARGE;

        if (m_repathTimer >= 0.3f || !m_formation.HasWaypoint()) {
            m_repathTimer = 0.0f;
            Vector2 targetDest = Vector2Add(enemyPos, Vector2Scale(MathUtils::SafeNormalize(toPlayer), dist - 15.0f));
            m_formation.SetOrder(targetDest, desiredHeading, true);
        }
    }
}
