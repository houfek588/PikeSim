#pragma once

#include "raylib.h"
#include "core/Types.h"
#include "simulation/Agent.h"
#include "formation/Rota.h"
#include <vector>
#include <string>

class SpatialHashGrid;

// --- TŘÍDA PRO PIKENÝRSKÝ ČTVEREC ---
class PikeSquare {
public:
    FormationId formationId{ 0 };
    Faction faction{ Faction::PLAYER };
    Vector2 position;       // Střed formace (pivot)
    float angle;            // Úhel rotace formace (v radiánech)
    float targetAngle;      // Cílový úhel rotace
    Vector2 targetPos;      // Cílová pozice (waypoint)
    bool hasWaypoint;       // Příznak aktivního waypointu
    bool hasTargetHeading;  // Příznak aktivního finálního srovnání čela
    float targetHeading;    // Cílové finální natočení čela (v radiánech)

    int cols;               // Počet sloupců (default 10)
    int rows;               // Počet řad (default 10)
    float spacing;          // Rozestupy mezi vojáky v pixelech (default 26.0f)
    PikeState pikeState;    // Taktický stav pík (PORTE / CHARGE)
    std::vector<Agent> soldiers; // Souvislé pole všech vojáků (105 celkem)
    std::vector<Rota> roty;      // Zástupy (roty) tvořící praporec
    float morale{ 1.0f };        // Morálka (0.0f - 1.0f)
    float stamina{ 1.0f };       // Stamina (0.0f - 1.0f)
    std::string name{ "Praporec" }; // Název formace

    // Konstruktor: inicializuje 10x10 pikenýrů + 4 NCO + 1 velitel
    PikeSquare(float x, float y, int c = 10, int r = 10, float space = 26.0f, Faction fac = Faction::PLAYER, float initAngle = -PI / 2.0f);

    // Geometrické výpočty
    Vector2 GetFrontCenter() const;
    void RotateAroundLocalPoint(float dAngle, Vector2 localPivot);
    void UpdateTargetSlots();

    // Správa zástupů (rot) a hierarchie formace
    const std::vector<Rota>& GetRoty() const { return roty; }
    std::vector<Rota>& GetRoty() { return roty; }
    void SwapRoty(int idxA, int idxB);
    void SetFormationDepth(int targetDepth);
    PikeSquare DetachRota(int rotaIndex);
    bool MergeFormation(PikeSquare& other);

    // Statistiky zbraní
    int GetPikemenCount() const;
    int GetMusketeerCount() const;
    int GetHalberdierCount() const;

    // Taktické a metrické ukazatele
    float GetNcoTempoFactor() const;
    float GetCohesionIndex() const;

    // Push of Pikes: Fyzika přenosu tlaku a deformace linie
    void UpdatePressurePropagation(float dt);
    float GetAverageFrontPressure() const;
    float GetLineDeformation() const;
    bool IsPushing() const { return m_isPushing; }
    float GetAccumulatedPushPressure() const { return m_accumulatedPushPressure; }

    // Bad War: Zlomení pík, boj zblízka a doplňování zástupů
    void UpdateMeleeAndBreakage(float dt, const SpatialHashGrid* grid);
    void UpdateRankReplenishment();
    int GetCasualtiesCount() const;
    int GetBrokenPikesCount() const;
    int GetLivingCount() const;

    // Manévry
    void Move(Vector2 dir, float dt, float speed);
    void Wheel(float dAngle, bool turnRight, float dt);
    void InPlaceTurn90(bool clockwise);
    void SetWaypoint(Vector2 point);
    void SetOrder(Vector2 point, float finalHeading, bool withHeading = true);
    void ClearWaypoint();

    // Hlavní simulační tick
    void Update(float dt, const SpatialHashGrid* grid = nullptr);

    // Přístupové metody
    Faction GetFaction() const { return faction; }
    void SetFaction(Faction f) { faction = f; }
    FormationId GetFormationId() const { return formationId; }
    void SetFormationId(FormationId id) { formationId = id; }
    float GetMorale() const { return morale; }
    void SetMorale(float m) { morale = m; }
    float GetStamina() const { return stamina; }
    void SetStamina(float s) { stamina = s; }
    const std::string& GetName() const { return name; }
    void SetName(const std::string& n) { name = n; }
    const std::vector<Agent>& GetSoldiers() const { return soldiers; }
    std::vector<Agent>& GetSoldiers() { return soldiers; }
    Vector2 GetPosition() const { return position; }
    float GetAngle() const { return angle; }
    PikeState GetState() const { return pikeState; }
    bool HasWaypoint() const { return hasWaypoint; }
    bool HasTargetHeading() const { return hasTargetHeading; }
    float GetTargetHeading() const { return targetHeading; }
    Vector2 GetWaypoint() const { return targetPos; }
    float GetSpacing() const { return spacing; }
    int GetCols() const { return cols; }
    int GetRows() const { return rows; }

private:
    // Předalokovaný buffer pro maticový obrat bez dynamických alokací v herním ticku
    std::vector<Agent> m_turnBuffer;

    // Push of Pikes stavové veličiny
    float m_accumulatedPushPressure{ 0.0f };
    float m_lineDeformation{ 0.0f };
    bool m_isPushing{ false };
};
