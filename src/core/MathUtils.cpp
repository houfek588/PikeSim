#include "core/MathUtils.h"

namespace MathUtils {

float NormalizeAngle(float angle) {
    angle = fmodf(angle + PI, TWO_PI);
    if (angle < 0.0f) angle += TWO_PI;
    return angle - PI;
}

void DrawLineDashed(Vector2 startPos, Vector2 endPos, float dashLength, float spaceLength, Color color) {
    if (dashLength + spaceLength <= 0.0f) return;
    Vector2 diff = Vector2Subtract(endPos, startPos);
    float len = Vector2Length(diff);
    if (len <= 0.0f) return;
    Vector2 dir = Vector2Normalize(diff);
    float currentLength = 0.0f;
    while (currentLength < len) {
        float drawLength = fminf(dashLength, len - currentLength);
        Vector2 dashStart = Vector2Add(startPos, Vector2Scale(dir, currentLength));
        Vector2 dashEnd = Vector2Add(dashStart, Vector2Scale(dir, drawLength));
        DrawLineV(dashStart, dashEnd, color);
        currentLength += dashLength + spaceLength;
    }
}

} // namespace MathUtils
