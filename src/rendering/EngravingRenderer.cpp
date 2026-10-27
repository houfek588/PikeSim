#include "rendering/EngravingRenderer.h"
#include "core/MathUtils.h"
#include <cmath>

EngravingRenderer::EngravingRenderer()
    : m_paperBase{ 235, 222, 200, 255 }
    , m_inkDark{ 45, 38, 34, 255 }
    , m_inkLight{ 120, 110, 100, 255 }
{
}

void EngravingRenderer::DrawBackground() {
    ClearBackground(m_paperBase);

    for (int i = 0; i < SCREEN_WIDTH; i += 40) {
        DrawLine(i, 0, i, SCREEN_HEIGHT, { 0, 0, 0, 12 });
    }
    for (int j = 0; j < SCREEN_HEIGHT; j += 40) {
        DrawLine(0, j, SCREEN_WIDTH, j, { 0, 0, 0, 12 });
    }
}

void EngravingRenderer::DrawWaypoint(Vector2 targetPos, Vector2 frontCenter) {
    DrawCircleLines(targetPos.x, targetPos.y, 8.0f, { 160, 50, 40, 150 });
    DrawCircle(targetPos.x, targetPos.y, 3.0f, { 160, 50, 40, 200 });
    MathUtils::DrawLineDashed(frontCenter, targetPos, 4.0f, 2.0f, { 160, 50, 40, 100 });
}

void EngravingRenderer::DrawAgent(const Agent& agent, float spacing, PikeState state) {
    Vector2 pos = agent.position;
    float r = spacing * 0.35f;

    // Padlý voják na bojišti
    if (!agent.isAlive) {
        DrawCircleSector(Vector2Add(pos, { 2, 2 }), r * 0.9f, 0, 360, 6, { 0, 0, 0, 25 });
        DrawCircleSector(pos, r * 0.75f, 0, 360, 6, { 170, 160, 150, 255 });
        DrawCircleLines(pos.x, pos.y, r * 0.75f, m_inkDark);
        DrawLine(pos.x - 3, pos.y - 3, pos.x + 3, pos.y + 3, m_inkDark);
        return;
    }

    Color plumeColor = (agent.faction == Faction::PLAYER) ? Color{ 70, 110, 170, 255 } : Color{ 170, 60, 50, 255 };

    if (agent.role == Role::LEADER) {
        plumeColor = (agent.faction == Faction::PLAYER) ? Color{ 40, 140, 230, 255 } : Color{ 210, 30, 20, 255 }; // Pero velitele
    } else if (agent.role == Role::NCO) {
        plumeColor = (agent.faction == Faction::PLAYER) ? Color{ 210, 160, 40, 255 } : Color{ 210, 90, 30, 255 }; // Pero poddůstojníka
    }

    // Kreslení stínu na papíře
    DrawCircleSector(Vector2Add(pos, { 2, 3 }), r, 0, 360, 8, { 0, 0, 0, 40 });

    // Tělo vojáka
    DrawCircleSector(pos, r, 0, 360, 12, m_inkLight);
    DrawCircleLines(pos.x, pos.y, r, m_inkDark);

    // Směrová čára ramen
    Vector2 leftShoulder = { pos.x - r * sinf(agent.angle), pos.y + r * cosf(agent.angle) };
    Vector2 rightShoulder = { pos.x + r * sinf(agent.angle), pos.y - r * cosf(agent.angle) };
    DrawLineEx(leftShoulder, rightShoulder, 1.5f, m_inkDark);

    // Přilba
    DrawCircle(pos.x, pos.y, r * 0.5f, { 220, 220, 220, 255 });
    DrawCircleLines(pos.x, pos.y, r * 0.5f, m_inkDark);

    // Plume (pero na přilbě)
    Vector2 plumeDir = { -cosf(agent.angle), -sinf(agent.angle) };
    Vector2 plumePos = Vector2Add(pos, Vector2Scale(plumeDir, r * 0.6f));
    DrawCircle(plumePos.x, plumePos.y, r * 0.3f, plumeColor);
    DrawCircleLines(plumePos.x, plumePos.y, r * 0.3f, m_inkDark);

    // Vykreslení zbraně
    float pikeLength = (state == PikeState::PORTE) ? spacing * 1.8f : (agent.isPikeBroken ? 18.0f : spacing * 3.5f);
    Vector2 pikeDir = { cosf(agent.pikeAngle), sinf(agent.pikeAngle) };

    Vector2 weaponStart = Vector2Add(pos, Vector2Scale({ sinf(agent.angle), -cosf(agent.angle) }, r * 0.4f));
    Vector2 weaponEnd = Vector2Add(weaponStart, Vector2Scale(pikeDir, pikeLength));

    DrawLineEx(Vector2Add(weaponStart, { 3, 5 }), Vector2Add(weaponEnd, { 3, 5 }), 1.0f, { 0, 0, 0, 30 });

    if (agent.role == Role::NCO) {
        // Poddůstojník má halapartnu
        float halberdLength = spacing * 1.5f;
        Vector2 halberdEnd = Vector2Add(weaponStart, Vector2Scale(pikeDir, halberdLength));
        DrawLineEx(weaponStart, halberdEnd, 1.8f, { 95, 75, 55, 255 });
        DrawLineEx(weaponStart, halberdEnd, 1.0f, m_inkDark);
        Vector2 headPos = Vector2Subtract(halberdEnd, Vector2Scale(pikeDir, 3.0f));
        DrawCircleSector(headPos, 4.0f, 0, 360, 4, { 180, 185, 190, 255 });
        DrawCircleSectorLines(headPos, 4.0f, 0, 360, 4, m_inkDark);
    } else if (agent.role == Role::LEADER) {
        // Velitel má tasený meč
        float swordLength = spacing * 1.1f;
        Vector2 swordEnd = Vector2Add(weaponStart, Vector2Scale(pikeDir, swordLength));
        DrawLineEx(weaponStart, swordEnd, 1.5f, { 200, 200, 205, 255 });
        DrawLineEx(weaponStart, swordEnd, 0.8f, m_inkDark);
    } else {
        // Klasická dlouhá píka (nebo zlomený pahýl)
        Color shaftColor = agent.isPikeInterlocked ? Color{ 85, 55, 35, 255 } : Color{ 115, 85, 60, 255 };
        float shaftWidth = agent.isPikeInterlocked ? 1.8f : 1.5f;
        DrawLineEx(weaponStart, weaponEnd, shaftWidth, shaftColor);
        DrawLineEx(weaponStart, weaponEnd, 0.8f, m_inkDark);

        if (agent.isPikeInterlocked && !agent.isPikeBroken) {
            // Dobové rytinové šrafování indikující mechanické napětí při zaklínění pík
            Vector2 mid1 = Vector2Add(weaponStart, Vector2Scale(pikeDir, pikeLength * 0.55f));
            Vector2 mid2 = Vector2Add(weaponStart, Vector2Scale(pikeDir, pikeLength * 0.75f));
            Vector2 normal = { -sinf(agent.pikeAngle) * 3.0f, cosf(agent.pikeAngle) * 3.0f };
            DrawLineV(Vector2Subtract(mid1, normal), Vector2Add(mid1, normal), m_inkDark);
            DrawLineV(Vector2Subtract(mid2, normal), Vector2Add(mid2, normal), m_inkDark);
        }

        if (!agent.isPikeBroken) {
            Vector2 tipBack = Vector2Subtract(weaponEnd, Vector2Scale(pikeDir, 6.0f));
            Vector2 tipLeft = { tipBack.x - 2.5f * sinf(agent.pikeAngle), tipBack.y + 2.5f * cosf(agent.pikeAngle) };
            Vector2 tipRight = { tipBack.x + 2.5f * sinf(agent.pikeAngle), tipBack.y - 2.5f * cosf(agent.pikeAngle) };

            DrawTriangle(weaponEnd, tipLeft, tipRight, { 190, 195, 200, 255 });
            DrawTriangleLines(weaponEnd, tipLeft, tipRight, m_inkDark);
        } else {
            // Roztřepený konec zlomeného dříku
            Vector2 normal = { -sinf(agent.pikeAngle) * 2.0f, cosf(agent.pikeAngle) * 2.0f };
            DrawLineV(Vector2Subtract(weaponEnd, normal), Vector2Add(weaponEnd, normal), m_inkDark);
        }
    }

    // Dobový poboční meč (Katzbalger) v boji zblízka
    if (agent.inMelee) {
        Vector2 swordDir = { cosf(agent.angle + 0.4f), sinf(agent.angle + 0.4f) };
        Vector2 swordEnd = Vector2Add(pos, Vector2Scale(swordDir, 16.0f));
        DrawLineEx(pos, swordEnd, 1.8f, m_inkDark);
        Vector2 guardPos = Vector2Add(pos, Vector2Scale(swordDir, 4.0f));
        Vector2 guardNorm = { -swordDir.y * 3.5f, swordDir.x * 3.5f };
        DrawLineV(Vector2Subtract(guardPos, guardNorm), Vector2Add(guardPos, guardNorm), m_inkDark);
    }
}

void EngravingRenderer::DrawFormation(const PikeSquare& square) {
    if (square.IsPushing() && square.cols > 1) {
        // Dobová linie tlaku čela formace
        for (int c = 0; c < square.cols - 1; ++c) {
            if (!square.roty[c].soldierIndices.empty() && !square.roty[c + 1].soldierIndices.empty()) {
                int idxA = square.roty[c].soldierIndices[0];
                int idxB = square.roty[c + 1].soldierIndices[0];
                DrawLineEx(square.soldiers[idxA].position, square.soldiers[idxB].position, 1.8f, m_inkDark);
            }
        }
    }

    for (const auto& soldier : square.soldiers) {
        DrawAgent(soldier, square.spacing, square.pikeState);
    }
}

void EngravingRenderer::DrawScene(const PikeSquare& square) {
    DrawBackground();

    if (square.hasWaypoint) {
        DrawWaypoint(square.targetPos, square.GetFrontCenter());
    }

    DrawFormation(square);
}
