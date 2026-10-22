#include "rendering/CameraController.h"
#include "raymath.h"
#include "core/Types.h"
#include <cmath>

CameraController::CameraController(Vector2 initialTarget) {
    m_camera.target = initialTarget;
    m_camera.offset = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;
}

void CameraController::Update(float dt) {
    HandleZoom();
    HandlePan(dt);
}

void CameraController::HandlePan(float dt) {
    // 1. Pan tažením prostředním tlačítkem myši (MMB)
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 delta = GetMouseDelta();
        m_camera.target.x -= delta.x / m_camera.zoom;
        m_camera.target.y -= delta.y / m_camera.zoom;
    }

    // 2. Pan klávesami (WASD nebo šipky)
    Vector2 keyMove = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) keyMove.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) keyMove.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) keyMove.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) keyMove.x += 1.0f;

    if (Vector2Length(keyMove) > 0.01f) {
        float len = Vector2Length(keyMove);
        keyMove.x /= len;
        keyMove.y /= len;

        float effectiveSpeed = m_panSpeed / m_camera.zoom;
        m_camera.target.x += keyMove.x * effectiveSpeed * dt;
        m_camera.target.y += keyMove.y * effectiveSpeed * dt;
    }
}

void CameraController::HandleZoom() {
    float wheel = GetMouseWheelMove();
    if (fabsf(wheel) > 0.001f) {
        // Zoom centrovaný na aktuální pozici kurzoru
        Vector2 mouseWorldBefore = ScreenToWorld(GetMousePosition());

        float factor = 1.0f + wheel * 0.15f;
        SetZoom(m_camera.zoom * factor);

        Vector2 mouseWorldAfter = ScreenToWorld(GetMousePosition());
        m_camera.target.x += (mouseWorldBefore.x - mouseWorldAfter.x);
        m_camera.target.y += (mouseWorldBefore.y - mouseWorldAfter.y);
    }
}

void CameraController::SetZoom(float z) {
    if (z < m_minZoom) z = m_minZoom;
    if (z > m_maxZoom) z = m_maxZoom;
    m_camera.zoom = z;
}

Vector2 CameraController::ScreenToWorld(Vector2 screenPos) const {
    return GetScreenToWorld2D(screenPos, m_camera);
}

Vector2 CameraController::WorldToScreen(Vector2 worldPos) const {
    return GetWorldToScreen2D(worldPos, m_camera);
}

void CameraController::CenterOn(Vector2 worldPos) {
    m_camera.target = worldPos;
}
