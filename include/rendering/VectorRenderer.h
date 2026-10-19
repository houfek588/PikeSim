#pragma once

#include "rendering/IRenderer.h"
#include "formation/PikeSquare.h"

// --- TAKTICKÝ VEKTOROVÝ RENDERER ---
class VectorRenderer : public IRenderer {
public:
    VectorRenderer();
    ~VectorRenderer() override = default;

    void DrawScene(const PikeSquare& square) override;
    void DrawBackground() override;
    void DrawFormation(const PikeSquare& square) override;
    void DrawAgent(const Agent& agent, float spacing, PikeState state) override;
    void DrawWaypoint(Vector2 targetPos, Vector2 frontCenter) override;

private:
    Color m_bgColor;
};
