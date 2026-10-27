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

bool LineSegmentIntersection(Vector2 p1, Vector2 p2, Vector2 p3, Vector2 p4,
                            Vector2* outIntersection,
                            float* outT1,
                            float* outT2) {
    float minX1 = fminf(p1.x, p2.x);
    float maxX1 = fmaxf(p1.x, p2.x);
    float minY1 = fminf(p1.y, p2.y);
    float maxY1 = fmaxf(p1.y, p2.y);

    float minX2 = fminf(p3.x, p4.x);
    float maxX2 = fmaxf(p3.x, p4.x);
    float minY2 = fminf(p3.y, p4.y);
    float maxY2 = fmaxf(p3.y, p4.y);

    const float eps = 1e-5f;
    if (maxX1 < minX2 - eps || maxX2 < minX1 - eps ||
        maxY1 < minY2 - eps || maxY2 < minY1 - eps) {
        return false;
    }

    Vector2 d1 = { p2.x - p1.x, p2.y - p1.y };
    Vector2 d2 = { p4.x - p3.x, p4.y - p3.y };

    float denom = d1.x * d2.y - d1.y * d2.x;
    if (fabsf(denom) < 1e-6f) {
        return false;
    }

    Vector2 delta = { p3.x - p1.x, p3.y - p1.y };
    float t1 = (delta.x * d2.y - delta.y * d2.x) / denom;
    float t2 = (delta.x * d1.y - delta.y * d1.x) / denom;

    if (t1 >= 0.0f && t1 <= 1.0f && t2 >= 0.0f && t2 <= 1.0f) {
        if (outIntersection) {
            outIntersection->x = p1.x + t1 * d1.x;
            outIntersection->y = p1.y + t1 * d1.y;
        }
        if (outT1) *outT1 = t1;
        if (outT2) *outT2 = t2;
        return true;
    }

    return false;
}

} // namespace MathUtils
