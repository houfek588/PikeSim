#pragma once

#include "raylib.h"
#include "core/Types.h"
#include "simulation/Agent.h"
#include <vector>

// --- TŘÍDA PRO PIKENÝRSKÝ ČTVEREC ---
class PikeSquare {
public:
    Vector2 position;       // Střed formace (pivot)
    float angle;            // Úhel rotace formace (v radiánech)
    float targetAngle;      // Cílový úhel rotace
    Vector2 targetPos;      // Cílová pozice (waypoint)
    bool hasWaypoint;       // Příznak aktivního waypointu

    int cols;               // Počet sloupců (default 10)
    int rows;               // Počet řad (default 10)
    float spacing;          // Rozestupy mezi vojáky v pixelech (default 26.0f)
    PikeState pikeState;    // Taktický stav pík (PORTE / CHARGE)
    std::vector<Agent> soldiers; // Souvislé pole všech vojáků (105 celkem)

    // Konstruktor: inicializuje 10x10 pikenýrů + 4 NCO + 1 velitel
    PikeSquare(float x, float y, int c = 10, int r = 10, float space = 26.0f);

    // Geometrické výpočty
    Vector2 GetFrontCenter() const;
    void RotateAroundLocalPoint(float dAngle, Vector2 localPivot);
    void UpdateTargetSlots();

    // Taktické a metrické ukazatele
    float GetNcoTempoFactor() const;
    float GetCohesionIndex() const;

    // Manévry
    void Move(Vector2 dir, float dt, float speed);
    void Wheel(float dAngle, bool turnRight, float dt);
    void InPlaceTurn90(bool clockwise);
    void SetWaypoint(Vector2 point);
    void ClearWaypoint();

    // Hlavní simulační tick
    void Update(float dt);

    // Přístupové metody
    const std::vector<Agent>& GetSoldiers() const { return soldiers; }
    std::vector<Agent>& GetSoldiers() { return soldiers; }
    Vector2 GetPosition() const { return position; }
    float GetAngle() const { return angle; }
    PikeState GetState() const { return pikeState; }
    bool HasWaypoint() const { return hasWaypoint; }
    Vector2 GetWaypoint() const { return targetPos; }
    float GetSpacing() const { return spacing; }
    int GetCols() const { return cols; }
    int GetRows() const { return rows; }

private:
    // Předalokovaný buffer pro maticový obrat bez dynamických alokací v herním ticku
    std::vector<Agent> m_turnBuffer;
};
