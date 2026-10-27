#pragma once

#include "raylib.h"
#include "raymath.h"
#include <cmath>

#ifndef TWO_PI
#define TWO_PI (2.0f * PI)
#endif

namespace MathUtils {

// Normalizuje úhel v radiánech do intervalu [-PI, PI]
float NormalizeAngle(float angle);

// Bezpečná normalizace vektoru (ochrana proti dělení nulou)
inline Vector2 SafeNormalize(Vector2 v, float epsilon = 0.0001f) {
    float len = Vector2Length(v);
    if (len <= epsilon) return { 0.0f, 0.0f };
    return Vector2Scale(v, 1.0f / len);
}

// Rotace 2D vektoru o zadaný úhel v radiánech
inline Vector2 RotateVector(Vector2 v, float angleRad) {
    float c = cosf(angleRad);
    float s = sinf(angleRad);
    return { v.x * c - v.y * s, v.x * s + v.y * c };
}

// Vykreslení přerušované čáry (Raylib utility)
void DrawLineDashed(Vector2 startPos, Vector2 endPos, float dashLength, float spaceLength, Color color);

// Detekce a výpočet průniku dvou úseček (p1 -> p2 a p3 -> p4)
bool LineSegmentIntersection(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4,
                            Vector2* outIntersection = nullptr,
                            float* outT1 = nullptr,
                            float* outT2 = nullptr);

} // namespace MathUtils

// Export do globálního jmenného prostoru pro zpětnou i přímou kompatibilitu
using MathUtils::NormalizeAngle;
using MathUtils::SafeNormalize;
using MathUtils::RotateVector;
using MathUtils::DrawLineDashed;
using MathUtils::LineSegmentIntersection;
