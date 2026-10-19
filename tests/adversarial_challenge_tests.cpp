#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <cassert>
#include <chrono>
#include <limits>
#include <iomanip>

#include "core/Types.h"
#include "core/MathUtils.h"
#include "simulation/Agent.h"
#include "formation/PikeSquare.h"

// Test reporting counters
static int g_passCount = 0;
static int g_failCount = 0;
static std::string g_currentTest = "";

#define CHALLENGE_TEST(name) \
    void name(); \
    struct Register_##name { \
        Register_##name() { g_currentTest = #name; } \
    }; \
    void name()

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << g_currentTest << " line " << __LINE__ << ": " << msg \
                      << " (Expression: " #cond ")\n"; \
            g_failCount++; \
            return; \
        } \
    } while(0)

#define ASSERT_FALSE(cond, msg) \
    do { \
        if (cond) { \
            std::cerr << "  [FAIL] " << g_currentTest << " line " << __LINE__ << ": " << msg \
                      << " (Expression: " #cond " should be false)\n"; \
            g_failCount++; \
            return; \
        } \
    } while(0)

#define ASSERT_NEAR(a, b, eps, msg) \
    do { \
        float diff = std::fabs(static_cast<float>(a) - static_cast<float>(b)); \
        if (diff > static_cast<float>(eps)) { \
            std::cerr << "  [FAIL] " << g_currentTest << " line " << __LINE__ << ": " << msg \
                      << " |" << (a) << " - " << (b) << "| = " << diff << " > " << (eps) << "\n"; \
            g_failCount++; \
            return; \
        } \
    } while(0)

#define CHALLENGE_PASS() \
    do { \
        g_passCount++; \
        std::cout << "  [PASS] " << g_currentTest << "\n"; \
    } while(0)

// ============================================================================
// SUITE 1: IN-PLACE MATRIX TURN STRESS (Shift+A / Shift+D)
// ============================================================================

// 1.1: 4 Consecutive CW Turns (Shift+D x 4) Returns Pikemen & NCOs to Exact Initial State
void Test_1_1_FourConsecutiveClockwiseTurns_Permutation() {
    g_currentTest = "Test_1_1_FourConsecutiveClockwiseTurns_Permutation";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    std::vector<int> initialIds(square.GetSoldiers().size());
    std::vector<Vector2> initialPositions(square.GetSoldiers().size());
    Vector2 initialPos = square.GetPosition();

    for (size_t i = 0; i < square.GetSoldiers().size(); ++i) {
        initialIds[i] = square.GetSoldiers()[i].id;
        initialPositions[i] = square.GetSoldiers()[i].position;
    }

    // 4 Consecutive 90-degree CW Turns
    for (int t = 1; t <= 4; ++t) {
        square.InPlaceTurn90(true);
    }

    // Check formation center position
    ASSERT_NEAR(square.GetPosition().x, initialPos.x, 0.001f, "Square position X altered after 360 rotation");
    ASSERT_NEAR(square.GetPosition().y, initialPos.y, 0.001f, "Square position Y altered after 360 rotation");

    // Check all 100 pikemen permutation identity
    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(square.GetSoldiers()[i].id == initialIds[i],
                    "Pikeman ID corrupted after 4 CW turns at index " + std::to_string(i));
        ASSERT_NEAR(square.GetSoldiers()[i].position.x, initialPositions[i].x, 0.01f,
                    "Pikeman pos X mismatch after 4 CW turns at index " + std::to_string(i));
        ASSERT_NEAR(square.GetSoldiers()[i].position.y, initialPositions[i].y, 0.01f,
                    "Pikeman pos Y mismatch after 4 CW turns at index " + std::to_string(i));
    }

    // Check all 4 NCOs permutation identity (indices 100, 101, 102, 103)
    for (int i = 100; i <= 103; ++i) {
        ASSERT_TRUE(square.GetSoldiers()[i].id == initialIds[i],
                    "NCO ID corrupted after 4 CW turns at index " + std::to_string(i));
        ASSERT_NEAR(square.GetSoldiers()[i].position.x, initialPositions[i].x, 0.01f,
                    "NCO pos X mismatch after 4 CW turns at index " + std::to_string(i));
        ASSERT_NEAR(square.GetSoldiers()[i].position.y, initialPositions[i].y, 0.01f,
                    "NCO pos Y mismatch after 4 CW turns at index " + std::to_string(i));
    }

    // Check Leader (index 104)
    ASSERT_TRUE(square.GetSoldiers()[104].id == initialIds[104], "Leader ID corrupted after 4 CW turns");
    ASSERT_NEAR(square.GetSoldiers()[104].position.x, initialPositions[104].x, 0.01f, "Leader pos X mismatch");
    ASSERT_NEAR(square.GetSoldiers()[104].position.y, initialPositions[104].y, 0.01f, "Leader pos Y mismatch");

    CHALLENGE_PASS();
}

// 1.2: 4 Consecutive CCW Turns (Shift+A x 4) Returns Pikemen & NCOs to Exact Initial State
void Test_1_2_FourConsecutiveCounterClockwiseTurns_Permutation() {
    g_currentTest = "Test_1_2_FourConsecutiveCounterClockwiseTurns_Permutation";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    std::vector<int> initialIds(square.GetSoldiers().size());
    std::vector<Vector2> initialPositions(square.GetSoldiers().size());

    for (size_t i = 0; i < square.GetSoldiers().size(); ++i) {
        initialIds[i] = square.GetSoldiers()[i].id;
        initialPositions[i] = square.GetSoldiers()[i].position;
    }

    // 4 Consecutive 90-degree CCW Turns
    for (int t = 1; t <= 4; ++t) {
        square.InPlaceTurn90(false);
    }

    // Check all 100 pikemen permutation identity
    for (int i = 0; i < 100; ++i) {
        ASSERT_TRUE(square.GetSoldiers()[i].id == initialIds[i],
                    "Pikeman ID corrupted after 4 CCW turns at index " + std::to_string(i));
        ASSERT_NEAR(square.GetSoldiers()[i].position.x, initialPositions[i].x, 0.01f,
                    "Pikeman pos X mismatch after 4 CCW turns at index " + std::to_string(i));
        ASSERT_NEAR(square.GetSoldiers()[i].position.y, initialPositions[i].y, 0.01f,
                    "Pikeman pos Y mismatch after 4 CCW turns at index " + std::to_string(i));
    }

    // Check all 4 NCOs
    for (int i = 100; i <= 103; ++i) {
        ASSERT_TRUE(square.GetSoldiers()[i].id == initialIds[i],
                    "NCO ID corrupted after 4 CCW turns at index " + std::to_string(i));
        ASSERT_NEAR(square.GetSoldiers()[i].position.x, initialPositions[i].x, 0.01f,
                    "NCO pos X mismatch after 4 CCW turns at index " + std::to_string(i));
        ASSERT_NEAR(square.GetSoldiers()[i].position.y, initialPositions[i].y, 0.01f,
                    "NCO pos Y mismatch after 4 CCW turns at index " + std::to_string(i));
    }

    CHALLENGE_PASS();
}

// 1.3: Angle Normalization Invariant Violation in InPlaceTurn90
void Test_1_3_AngleNormalizationInvariantInTurn() {
    g_currentTest = "Test_1_3_AngleNormalizationInvariantInTurn";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    // Initial angle is -PI/2 (-1.5707963)
    // 4 CW turns add 4 * (PI/2) = 2*PI. The angle becomes 3*PI/2 (4.712389).
    for (int t = 0; t < 4; ++t) {
        square.InPlaceTurn90(true);
    }

    // Rule 4 of AGENTS.md mandates: "Úhly vždy normalizujte do intervalu [-PI, PI]"
    // Check if angle is within [-PI, PI] without external normalization
    float angle = square.GetAngle();
    std::cout << "    [DIAGNOSTIC] Formation angle after 4 CW turns: " << angle << " rad (expected in [-PI, PI])\n";
    ASSERT_TRUE(angle >= -PI && angle <= PI,
                "InPlaceTurn90 fails AGENTS.md Invariant 4: angle not normalized in [-PI, PI] after turns!");

    CHALLENGE_PASS();
}

// 1.4: Target Slots Refreshed Immediately inside InPlaceTurn90
void Test_1_4_TargetSlotsUpdatedImmediately() {
    g_currentTest = "Test_1_4_TargetSlotsUpdatedImmediately";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    square.InPlaceTurn90(true);

    // InPlaceTurn90 rotates the square and permutes the soldiers,
    // but fails to call UpdateTargetSlots(). Thus soldiers[0].targetSlot is stale!
    float distToSlot = Vector2Distance(square.GetSoldiers()[0].position, square.GetSoldiers()[0].targetSlot);
    std::cout << "    [DIAGNOSTIC] Soldier 0 distance to targetSlot immediately after turn: " << distToSlot << " px\n";
    ASSERT_NEAR(distToSlot, 0.0f, 0.1f,
                "InPlaceTurn90 leaves stale targetSlot on soldiers before Update() is called!");

    CHALLENGE_PASS();
}

// 1.5: 1000-Turn Randomized Stress Test
void Test_1_5_RandomizedTurnStress() {
    g_currentTest = "Test_1_5_RandomizedTurnStress";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    std::vector<int> initialIds(square.GetSoldiers().size());
    for (size_t i = 0; i < square.GetSoldiers().size(); ++i) {
        initialIds[i] = square.GetSoldiers()[i].id;
    }

    int netTurns = 0;
    unsigned int rngState = 123456789;
    auto nextRand = [&rngState]() {
        rngState = rngState * 1664525u + 1013904223u;
        return rngState;
    };

    for (int step = 0; step < 1000; ++step) {
        bool cw = (nextRand() % 2) == 1;
        square.InPlaceTurn90(cw);
        if (cw) netTurns = (netTurns + 1) % 4;
        else    netTurns = (netTurns + 3) % 4;
    }

    while (netTurns != 0) {
        square.InPlaceTurn90(false);
        netTurns = (netTurns + 3) % 4;
    }

    for (size_t i = 0; i < 104; ++i) {
        ASSERT_TRUE(square.GetSoldiers()[i].id == initialIds[i],
                    "Permutation drift after 1000 randomized turns at index " + std::to_string(i));
    }

    CHALLENGE_PASS();
}

// ============================================================================
// SUITE 2: MANEUVER & WAYPOINT CONFLICT
// ============================================================================

// 2.1: InPlaceTurn90 Cancels Active Waypoint
void Test_2_1_InPlaceTurnCancelsWaypoint() {
    g_currentTest = "Test_2_1_InPlaceTurnCancelsWaypoint";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    square.SetWaypoint({ 800.0f, 200.0f });
    ASSERT_TRUE(square.HasWaypoint(), "Waypoint should be active");

    square.InPlaceTurn90(true);
    ASSERT_FALSE(square.HasWaypoint(), "InPlaceTurn90 must cancel active waypoint");

    square.SetWaypoint({ 800.0f, 200.0f });
    square.InPlaceTurn90(false);
    ASSERT_FALSE(square.HasWaypoint(), "InPlaceTurn90 CCW must cancel active waypoint");

    CHALLENGE_PASS();
}

// 2.2: Corner Wheel Maneuver Cancels or Replaces Active Waypoint
void Test_2_2_WheelConflictWithWaypoint() {
    g_currentTest = "Test_2_2_WheelConflictWithWaypoint";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    square.SetWaypoint({ 800.0f, 200.0f });
    ASSERT_TRUE(square.HasWaypoint(), "Waypoint should be active");

    // Initiate Wheel() maneuver
    square.Wheel(0.05f, true, 0.016f);

    // In tactical Renaissance maneuvering, manual wheel command should cancel active waypoint
    ASSERT_FALSE(square.HasWaypoint(), "Manual Wheel() maneuver fails to cancel active waypoint!");

    CHALLENGE_PASS();
}

// 2.3: Manual Move Maneuver Cancels Active Waypoint
void Test_2_3_ManualMoveConflictWithWaypoint() {
    g_currentTest = "Test_2_3_ManualMoveConflictWithWaypoint";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    square.SetWaypoint({ 800.0f, 200.0f });
    ASSERT_TRUE(square.HasWaypoint(), "Waypoint should be active");

    square.Move({ 1.0f, 0.0f }, 0.016f, 80.0f);
    ASSERT_FALSE(square.HasWaypoint(), "Manual Move() maneuver fails to cancel active waypoint!");

    CHALLENGE_PASS();
}

// 2.4: Waypoint at Front Center (Near-zero distance)
void Test_2_4_WaypointZeroDistance() {
    g_currentTest = "Test_2_4_WaypointZeroDistance";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    Vector2 frontCenter = square.GetFrontCenter();
    square.SetWaypoint(frontCenter);
    ASSERT_TRUE(square.HasWaypoint(), "Waypoint should be set");

    square.Update(0.016f);

    ASSERT_FALSE(square.HasWaypoint(), "Waypoint should be cleared upon arrival");
    ASSERT_FALSE(std::isnan(square.GetPosition().x), "Position X became NaN");
    ASSERT_FALSE(std::isnan(square.GetPosition().y), "Position Y became NaN");
    ASSERT_FALSE(std::isnan(square.GetAngle()), "Angle became NaN");

    CHALLENGE_PASS();
}

// ============================================================================
// SUITE 3: NUMERICAL STRESS & STABILITY
// ============================================================================

// 3.1: Sustained Delta Time (dt = 0.5s) Stationary SwingTimer Explosion
void Test_3_1_SustainedLargeDeltaTimeSwingTimerExplosion() {
    g_currentTest = "Test_3_1_SustainedLargeDeltaTimeSwingTimerExplosion";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    // Stop all soldiers (agentSpeed = 0 <= 5.0)
    for (auto& a : square.GetSoldiers()) {
        a.velocity = { 0.0f, 0.0f };
        a.swingTimer = 1.0f; // Initial non-zero sway
    }

    // Run 250 ticks with dt = 0.5s
    // In code: swingTimer = Lerp(swingTimer, 0.0f, 5.0f * dt)
    // When dt = 0.5, amount = 2.5 > 1.0. swingTimer_{n+1} = -1.5 * swingTimer_n!
    // After 250 ticks, (-1.5)^250 explodes to Infinity, and sinf(inf) returns NaN!
    for (int step = 0; step < 250; ++step) {
        square.Update(0.5f);
    }

    float t0 = square.GetSoldiers()[0].swingTimer;
    float pike0 = square.GetSoldiers()[0].pikeAngle;
    std::cout << "    [DIAGNOSTIC] swingTimer after 250 ticks at dt=0.5: " << t0 << ", pikeAngle: " << pike0 << "\n";

    ASSERT_FALSE(std::isnan(t0), "swingTimer exploded to NaN via Lerp divergence!");
    ASSERT_FALSE(std::isinf(t0), "swingTimer exploded to Inf via Lerp divergence!");
    ASSERT_FALSE(std::isnan(pike0), "pikeAngle became NaN due to swingTimer explosion!");

    CHALLENGE_PASS();
}

// 3.2: Extreme Large Delta Time (dt = 10.0s) Single Step
void Test_3_2_ExtremeDeltaTimeStability() {
    g_currentTest = "Test_3_2_ExtremeDeltaTimeStability";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    square.Update(10.0f);
    for (const auto& a : square.GetSoldiers()) {
        ASSERT_FALSE(std::isnan(a.position.x), "Agent position X became NaN at dt=10.0");
        ASSERT_FALSE(std::isnan(a.velocity.x), "Agent velocity X became NaN at dt=10.0");
        ASSERT_FALSE(std::isnan(a.swingTimer), "Agent swingTimer became NaN at dt=10.0");
        ASSERT_FALSE(std::isinf(a.swingTimer), "Agent swingTimer exploded to Inf at dt=10.0");
    }

    CHALLENGE_PASS();
}

// 3.3: Speed Clamping Under Extreme Displacements
void Test_3_3_SpeedClampingEnforcement() {
    g_currentTest = "Test_3_3_SpeedClampingEnforcement";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    for (auto& a : square.GetSoldiers()) {
        a.position = { 1000000.0f, 1000000.0f };
        a.velocity = { 50000.0f, -50000.0f };
    }

    square.Update(0.016f);

    for (size_t i = 0; i < square.GetSoldiers().size(); ++i) {
        float spd = Vector2Length(square.GetSoldiers()[i].velocity);
        ASSERT_FALSE(std::isnan(spd), "Speed is NaN");
        ASSERT_TRUE(spd <= 60.0f + 1.0f, "Speed clamp exceeded! spd = " + std::to_string(spd));
    }

    CHALLENGE_PASS();
}

// 3.4: Infinite Loop Vulnerability in MathUtils::NormalizeAngle
void Test_3_4_AngleNormalizationInfiniteLoopVulnerability() {
    g_currentTest = "Test_3_4_AngleNormalizationInfiniteLoopVulnerability";

    // In 32-bit float arithmetic, if angle >= 1.68e7f, adding/subtracting 2*PI (6.28)
    // rounds back to the original angle because 2*PI is below machine epsilon.
    // Therefore `while (angle > PI) angle -= 2.0f * PI;` loops infinitely and locks the CPU!
    // To verify this without hanging the test process, we test float arithmetic:
    float bigAngle = 2.0e8f;
    float afterSubtract = bigAngle - 2.0f * PI;
    std::cout << "    [DIAGNOSTIC] (2e8f - 2*PI) == 2e8f is " << ((bigAngle == afterSubtract) ? "TRUE (Infinite Loop Hazard!)" : "FALSE") << "\n";

    // If angle is 2e8f, while(angle > PI) angle -= 2*PI will loop forever!
    ASSERT_TRUE(bigAngle != afterSubtract,
                "NormalizeAngle has a catastrophic INFINITE LOOP flaw on large floats: (2e8f - 2*PI == 2e8f) causes while loop to hang forever!");

    CHALLENGE_PASS();
}

// 3.5: NCO Tempo Factor Robustness
void Test_3_5_NcoTempoFactorRobustness() {
    g_currentTest = "Test_3_5_NcoTempoFactorRobustness";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    square.GetSoldiers()[100].position = { 1e7f, 1e7f };
    float factor = square.GetNcoTempoFactor();
    ASSERT_FALSE(std::isnan(factor), "Tempo factor became NaN on extreme NCO position");
    ASSERT_NEAR(factor, 0.15f, 0.001f, "Tempo factor should clamp to 0.15f at extreme distance");

    CHALLENGE_PASS();
}

// ============================================================================
// SUITE 4: EMPIRICAL HARNESS FOR TIER 4 TEST FAILURE INVESTIGATION
// ============================================================================

// 4.1: Formation Cohesion Recovery Dynamics after March
void Test_4_1_CohesionRecoveryDynamics() {
    g_currentTest = "Test_4_1_CohesionRecoveryDynamics";
    PikeSquare square(600.0f, 400.0f, 10, 10, 26.0f);

    ASSERT_NEAR(square.GetCohesionIndex(), 1.0f, 0.001f, "Initial cohesion must be 1.0");

    // March forward 40 ticks at speed 80
    for (int i = 0; i < 40; ++i) {
        square.Move({ 0.0f, -1.0f }, 0.016f, 80.0f);
        square.Update(0.016f);
    }

    float cohesionAfterMarch = square.GetCohesionIndex();
    std::cout << "    [DIAGNOSTIC] Cohesion immediately after 40 march steps: " << cohesionAfterMarch << "\n";

    // Relaxation phase: 60 ticks
    for (int i = 0; i < 60; ++i) {
        square.Update(0.016f);
    }
    float cohesionAfter60Relax = square.GetCohesionIndex();
    std::cout << "    [DIAGNOSTIC] Cohesion after 60 relaxation ticks: " << cohesionAfter60Relax << "\n";

    // Relaxation phase: 200 ticks
    for (int i = 0; i < 140; ++i) {
        square.Update(0.016f);
    }
    float cohesionAfter200Relax = square.GetCohesionIndex();
    std::cout << "    [DIAGNOSTIC] Cohesion after 200 relaxation ticks: " << cohesionAfter200Relax << "\n";

    // Relaxation phase: 500 ticks
    for (int i = 0; i < 300; ++i) {
        square.Update(0.016f);
    }
    float cohesionAfter500Relax = square.GetCohesionIndex();
    std::cout << "    [DIAGNOSTIC] Cohesion after 500 relaxation ticks: " << cohesionAfter500Relax << "\n";

    // Verify whether cohesion reaches >= 0.95 in 60 ticks vs longer
    ASSERT_TRUE(cohesionAfter60Relax >= 0.95f,
                "Cohesion after 60 ticks is only " + std::to_string(cohesionAfter60Relax) + " (< 0.95)");

    CHALLENGE_PASS();
}

int main() {
    std::cout << "=========================================================\n";
    std::cout << "     EMPIRICAL ADVERSARIAL CHALLENGE TEST SUITE          \n";
    std::cout << "=========================================================\n\n";

    std::cout << "--- SUITE 1: IN-PLACE MATRIX TURN STRESS ---\n";
    Test_1_1_FourConsecutiveClockwiseTurns_Permutation();
    Test_1_2_FourConsecutiveCounterClockwiseTurns_Permutation();
    Test_1_3_AngleNormalizationInvariantInTurn();
    Test_1_4_TargetSlotsUpdatedImmediately();
    Test_1_5_RandomizedTurnStress();

    std::cout << "\n--- SUITE 2: MANEUVER & WAYPOINT CONFLICT ---\n";
    Test_2_1_InPlaceTurnCancelsWaypoint();
    Test_2_2_WheelConflictWithWaypoint();
    Test_2_3_ManualMoveConflictWithWaypoint();
    Test_2_4_WaypointZeroDistance();

    std::cout << "\n--- SUITE 3: NUMERICAL STRESS & STABILITY ---\n";
    Test_3_1_SustainedLargeDeltaTimeSwingTimerExplosion();
    Test_3_2_ExtremeDeltaTimeStability();
    Test_3_3_SpeedClampingEnforcement();
    Test_3_4_AngleNormalizationInfiniteLoopVulnerability();
    Test_3_5_NcoTempoFactorRobustness();

    std::cout << "\n--- SUITE 4: TIER 4 TEST HARNESS INVESTIGATION ---\n";
    Test_4_1_CohesionRecoveryDynamics();

    std::cout << "\n=========================================================\n";
    std::cout << "CHALLENGE RESULTS SUMMARY:\n";
    std::cout << "  Passed: " << g_passCount << "\n";
    std::cout << "  Failed: " << g_failCount << "\n";
    std::cout << "=========================================================\n";

    return (g_failCount == 0) ? 0 : 1;
}
