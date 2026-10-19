#pragma once

#include "rendering/IRenderer.h"
#include "formation/PikeSquare.h"

// --- HISTORICKÝ RENDERER V PAPÍROVÉM MĚDIRYTECKÉM STYLU ---
class EngravingRenderer : public IRenderer {
public:
    EngravingRenderer();
    ~EngravingRenderer() override = default;

    void DrawScene(const PikeSquare& square) override;
    void DrawBackground() override;
    void DrawFormation(const PikeSquare& square) override;
    void DrawAgent(const Agent& agent, float spacing, PikeState state) override;
    void DrawWaypoint(Vector2 targetPos, Vector2 frontCenter) override;

private:
    Color m_paperBase;
    Color m_inkDark;
    Color m_inkLight;
};
