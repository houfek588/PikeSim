#pragma once

#include "raylib.h"

// --- 2D KAMEROVÝ SYSTÉM PRO VOLNÝ POHYB PO BOJIŠTI ---
class CameraController {
public:
    CameraController(Vector2 initialTarget = { 600.0f, 400.0f });

    void Update(float dt);
    void HandlePan(float dt);
    void HandleZoom();

    // Převody souřadnic mezi obrazovkou a herním světem
    Vector2 ScreenToWorld(Vector2 screenPos) const;
    Vector2 WorldToScreen(Vector2 worldPos) const;

    // Přemístění kamery na zadané souřadnice
    void CenterOn(Vector2 worldPos);

    // Přístup k Raylib Camera2D struktuře
    const Camera2D& GetCamera() const { return m_camera; }
    Camera2D& GetCamera() { return m_camera; }

    // Gettery a settery
    float GetZoom() const { return m_camera.zoom; }
    void SetZoom(float z);
    Vector2 GetTarget() const { return m_camera.target; }
    void SetTarget(Vector2 target) { m_camera.target = target; }
    Vector2 GetOffset() const { return m_camera.offset; }
    void SetOffset(Vector2 offset) { m_camera.offset = offset; }

    float GetMinZoom() const { return m_minZoom; }
    float GetMaxZoom() const { return m_maxZoom; }

private:
    Camera2D m_camera;
    float m_minZoom{ 0.3f };
    float m_maxZoom{ 2.5f };
    float m_panSpeed{ 600.0f };
    bool m_isDraggingMiddle{ false };
    Vector2 m_prevMousePos{ 0.0f, 0.0f };
};
