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
    Color plumeColor = m_inkLight;

    if (agent.role == Role::LEADER) {
        plumeColor = { 180, 50, 40, 255 }; // Výrazné červené pero velitele
    } else if (agent.role == Role::NCO) {
        plumeColor = { 190, 130, 40, 255 }; // Oranžové pero poddůstojníka
    }

    Vector2 pos = agent.position;
    float r = spacing * 0.35f;

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
    float pikeLength = (state == PikeState::PORTE) ? spacing * 1.8f : spacing * 3.5f;
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
        // Klasická dlouhá píka
        DrawLineEx(weaponStart, weaponEnd, 1.5f, { 115, 85, 60, 255 });
        DrawLineEx(weaponStart, weaponEnd, 0.8f, m_inkDark);

        Vector2 tipBack = Vector2Subtract(weaponEnd, Vector2Scale(pikeDir, 6.0f));
        Vector2 tipLeft = { tipBack.x - 2.5f * sinf(agent.pikeAngle), tipBack.y + 2.5f * cosf(agent.pikeAngle) };
        Vector2 tipRight = { tipBack.x + 2.5f * sinf(agent.pikeAngle), tipBack.y - 2.5f * cosf(agent.pikeAngle) };

        DrawTriangle(weaponEnd, tipLeft, tipRight, { 190, 195, 200, 255 });
        DrawTriangleLines(weaponEnd, tipLeft, tipRight, m_inkDark);
    }
}

void EngravingRenderer::DrawFormation(const PikeSquare& square) {
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
