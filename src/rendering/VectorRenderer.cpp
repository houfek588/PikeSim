#include "rendering/VectorRenderer.h"
#include "core/MathUtils.h"
#include <cmath>

VectorRenderer::VectorRenderer()
    : m_bgColor{ 15, 17, 22, 255 }
{
}

void VectorRenderer::DrawBackground() {
    ClearBackground(m_bgColor);
}

void VectorRenderer::DrawWaypoint(Vector2 targetPos, Vector2 frontCenter) {
    DrawCircleLines(targetPos.x, targetPos.y, 12.0f, { 0, 255, 255, 200 });
    DrawLineV(frontCenter, targetPos, { 0, 255, 255, 100 });
}

void VectorRenderer::DrawAgent(const Agent& agent, float spacing, PikeState state) {
    Color coreColor = (agent.faction == Faction::PLAYER) ? Color{ 0, 200, 255, 255 } : Color{ 230, 50, 50, 255 };
    if (agent.role == Role::LEADER) {
        coreColor = (agent.faction == Faction::PLAYER) ? Color{ 255, 255, 255, 255 } : Color{ 255, 20, 20, 255 };
    } else if (agent.role == Role::NCO) {
        coreColor = (agent.faction == Faction::PLAYER) ? Color{ 255, 180, 0, 255 } : Color{ 255, 120, 0, 255 };
    }

    Vector2 pos = agent.position;
    float r = spacing * 0.25f;

    DrawCircleLines(agent.targetSlot.x, agent.targetSlot.y, 3.0f, { 100, 100, 100, 100 });
    DrawLineV(pos, agent.targetSlot, { 255, 255, 255, 50 });

    DrawCircleV(pos, r, coreColor);
    DrawCircleLines(pos.x, pos.y, r + 1, { 255, 255, 255, 150 });

    Vector2 velEnd = Vector2Add(pos, Vector2Scale(agent.velocity, 0.15f));
    DrawLineV(pos, velEnd, { 0, 255, 100, 180 });

    float pikeLength = (state == PikeState::PORTE) ? spacing * 1.5f : spacing * 3.2f;
    Vector2 pikeDir = { cosf(agent.pikeAngle), sinf(agent.pikeAngle) };
    Vector2 weaponEnd = Vector2Add(pos, Vector2Scale(pikeDir, pikeLength));

    float speed = Vector2Length(agent.velocity);
    Color pikeColor = (agent.faction == Faction::PLAYER) ? Color{ 150, 230, 150, 180 } : Color{ 255, 180, 120, 180 };
    if (speed > 40.0f) {
        pikeColor = { 255, 230, 100, 200 };
    }

    DrawLineEx(pos, weaponEnd, 1.2f, pikeColor);
    DrawCircleV(weaponEnd, 2.0f, { 255, 255, 255, 225 });
}

void VectorRenderer::DrawFormation(const PikeSquare& square) {
    if (square.soldiers.size() >= static_cast<size_t>(square.rows * square.cols)) {
        // Výpočet mřížky pro 100 pikenýrů (0 až 99)
        Vector2 topLeft = square.soldiers[0].targetSlot;
        Vector2 topRight = square.soldiers[square.cols - 1].targetSlot;
        Vector2 bottomLeft = square.soldiers[(square.rows - 1) * square.cols].targetSlot;
        Vector2 bottomRight = square.soldiers[square.rows * square.cols - 1].targetSlot;

        DrawLineV(topLeft, topRight, { 0, 255, 100, 60 });
        DrawLineV(topRight, bottomRight, { 0, 255, 100, 60 });
        DrawLineV(bottomRight, bottomLeft, { 0, 255, 100, 60 });
        DrawLineV(bottomLeft, topLeft, { 0, 255, 100, 60 });
    }

    for (const auto& soldier : square.soldiers) {
        DrawAgent(soldier, square.spacing, square.pikeState);
    }
}

void VectorRenderer::DrawScene(const PikeSquare& square) {
    DrawBackground();
    DrawFormation(square);

    if (square.hasWaypoint) {
        DrawWaypoint(square.targetPos, square.GetFrontCenter());
    }
}
