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
    Vector2 pos = agent.position;
    float r = spacing * 0.25f;

    // Vykreslení padlého vojáka
    if (!agent.isAlive) {
        DrawCircleV(pos, r * 0.8f, { 50, 50, 55, 160 });
        DrawLine(pos.x - 3, pos.y - 3, pos.x + 3, pos.y + 3, { 180, 40, 40, 200 });
        DrawLine(pos.x - 3, pos.y + 3, pos.x + 3, pos.y - 3, { 180, 40, 40, 200 });
        return;
    }

    Color coreColor = (agent.faction == Faction::PLAYER) ? Color{ 0, 200, 255, 255 } : Color{ 230, 50, 50, 255 };
    if (agent.role == Role::LEADER) {
        coreColor = (agent.faction == Faction::PLAYER) ? Color{ 255, 255, 255, 255 } : Color{ 255, 20, 20, 255 };
    } else if (agent.role == Role::NCO) {
        coreColor = (agent.faction == Faction::PLAYER) ? Color{ 255, 180, 0, 255 } : Color{ 255, 120, 0, 255 };
    }

    DrawCircleLines(agent.targetSlot.x, agent.targetSlot.y, 3.0f, { 100, 100, 100, 100 });
    DrawLineV(pos, agent.targetSlot, { 255, 255, 255, 50 });

    DrawCircleV(pos, r, coreColor);
    DrawCircleLines(pos.x, pos.y, r + 1, { 255, 255, 255, 150 });

    Vector2 velEnd = Vector2Add(pos, Vector2Scale(agent.velocity, 0.15f));
    DrawLineV(pos, velEnd, { 0, 255, 100, 180 });

    // Vykreslení zbraně (nebo zlomené píky)
    float pikeLength = (state == PikeState::PORTE) ? spacing * 1.5f : (agent.isPikeBroken ? 18.0f : spacing * 3.2f);
    Vector2 pikeDir = { cosf(agent.pikeAngle), sinf(agent.pikeAngle) };
    Vector2 weaponEnd = Vector2Add(pos, Vector2Scale(pikeDir, pikeLength));

    float speed = Vector2Length(agent.velocity);
    Color pikeColor = (agent.faction == Faction::PLAYER) ? Color{ 150, 230, 150, 180 } : Color{ 255, 180, 120, 180 };
    if (agent.isPikeBroken) {
        pikeColor = Color{ 180, 100, 50, 220 }; // Pahýl zlomené píky
    } else if (agent.isPikeInterlocked) {
        pikeColor = Color{ 255, 130, 40, 240 }; // Oranžovo-červený akcent napětí při zaklesnutí
    } else if (speed > 40.0f) {
        pikeColor = { 255, 230, 100, 200 };
    }

    float lineWidth = agent.isPikeInterlocked ? 1.8f : 1.2f;
    DrawLineEx(pos, weaponEnd, lineWidth, pikeColor);
    if (!agent.isPikeBroken) {
        DrawCircleV(weaponEnd, agent.isPikeInterlocked ? 3.0f : 2.0f, agent.isPikeInterlocked ? Color{ 255, 230, 80, 255 } : Color{ 255, 255, 255, 225 });
    }

    // Tasený poboční meč (Katzbalger) v boji zblízka
    if (agent.inMelee) {
        Vector2 swordDir = { cosf(agent.angle + 0.3f), sinf(agent.angle + 0.3f) };
        Vector2 swordEnd = Vector2Add(pos, Vector2Scale(swordDir, 16.0f));
        DrawLineEx(pos, swordEnd, 1.8f, { 240, 240, 250, 240 });
    }

    if (agent.pressureForce > 20.0f) {
        float pNorm = fminf(agent.pressureForce / 225.0f, 1.0f);
        Color pressureColor = { static_cast<unsigned char>(100 + 155 * pNorm), static_cast<unsigned char>(255 * (1.0f - pNorm * 0.7f)), 50, 180 };
        DrawCircleLines(pos.x, pos.y, r + 2.0f, pressureColor);
    }
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

    if (square.IsPushing() && square.cols > 1) {
        for (int c = 0; c < square.cols - 1; ++c) {
            if (!square.roty[c].soldierIndices.empty() && !square.roty[c + 1].soldierIndices.empty()) {
                int idxA = square.roty[c].soldierIndices[0];
                int idxB = square.roty[c + 1].soldierIndices[0];
                DrawLineEx(square.soldiers[idxA].position, square.soldiers[idxB].position, 2.5f, { 255, 80, 40, 220 });
            }
        }
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
