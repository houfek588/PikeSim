#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <sstream>
#include <memory>
#include <cassert>
#include <fstream>

#include "core/Types.h"
#include "core/MathUtils.h"
#include "simulation/Agent.h"
#include "formation/PikeSquare.h"
#include "rendering/IRenderer.h"
#include "rendering/EngravingRenderer.h"
#include "rendering/VectorRenderer.h"
#include "ui/UIManager.h"

// --- TEST FRAMEWORK HARNESS ---
static int g_testsPassed = 0;
static int g_testsFailed = 0;
static std::string g_currentTestName = "";

#define TEST_CASE(name) \
    void name(); \
    struct Register_##name { \
        Register_##name() { \
            g_currentTestName = #name; \
        } \
    }; \
    void name()

#define EXPECT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << g_currentTestName << " line " << __LINE__ << ": EXPECT_TRUE(" #cond ") failed!\n"; \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define EXPECT_FALSE(cond) \
    do { \
        if (cond) { \
            std::cerr << "  [FAIL] " << g_currentTestName << " line " << __LINE__ << ": EXPECT_FALSE(" #cond ") failed!\n"; \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define EXPECT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            std::cerr << "  [FAIL] " << g_currentTestName << " line " << __LINE__ << ": EXPECT_EQ(" #a ", " #b ") failed: (" \
                      << (a) << " != " << (b) << ")\n"; \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define EXPECT_NEAR(a, b, eps) \
    do { \
        float diff = std::fabs(static_cast<float>(a) - static_cast<float>(b)); \
        if (diff > static_cast<float>(eps)) { \
            std::cerr << "  [FAIL] " << g_currentTestName << " line " << __LINE__ << ": EXPECT_NEAR(" #a ", " #b ", " #eps ") failed: |" \
                      << (a) << " - " << (b) << "| = " << diff << " > " << (eps) << "\n"; \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        g_testsPassed++; \
        std::cout << "  [PASS] " << g_currentTestName << "\n"; \
    } while(0)

// ============================================================================
// TIER 1: FEATURE COVERAGE (Isolated Happy Path)
// ============================================================================

// Feature 1: Screen & Window Constants
void Test_F1_ScreenConstants() {
    g_currentTestName = "Test_F1_ScreenConstants";
    EXPECT_EQ(SCREEN_WIDTH, 1200);
    EXPECT_EQ(SCREEN_HEIGHT, 800);
    TEST_PASS();
}

// Feature 2: Role Enumeration
void Test_F2_RoleEnumeration() {
    g_currentTestName = "Test_F2_RoleEnumeration";
    Role p = Role::PIKEMAN;
    Role n = Role::NCO;
    Role l = Role::LEADER;
    EXPECT_TRUE(p != n);
    EXPECT_TRUE(n != l);
    EXPECT_TRUE(p != l);
    TEST_PASS();
}

// Feature 3: PikeState Enumeration
void Test_F3_PikeStateEnumeration() {
    g_currentTestName = "Test_F3_PikeStateEnumeration";
    PikeState s1 = PikeState::PORTE;
    PikeState s2 = PikeState::CHARGE;
    EXPECT_TRUE(s1 != s2);
    TEST_PASS();
}

// Feature 4: VisualMode Enumeration
void Test_F4_VisualModeEnumeration() {
    g_currentTestName = "Test_F4_VisualModeEnumeration";
    VisualMode m1 = VisualMode::ENGRAVING;
    VisualMode m2 = VisualMode::VECTOR;
    EXPECT_TRUE(m1 != m2);
    TEST_PASS();
}

// Feature 5: Angle Normalization
void Test_F5_AngleNormalization() {
    g_currentTestName = "Test_F5_AngleNormalization";
    EXPECT_NEAR(MathUtils::NormalizeAngle(0.0f), 0.0f, 0.0001f);
    EXPECT_NEAR(MathUtils::NormalizeAngle(PI / 2.0f), PI / 2.0f, 0.0001f);
    EXPECT_NEAR(MathUtils::NormalizeAngle(-PI / 2.0f), -PI / 2.0f, 0.0001f);
    
    // Values outside [-PI, PI]
    float wrapped1 = MathUtils::NormalizeAngle(2.5f * PI); // 2.5 PI -> 0.5 PI
    EXPECT_NEAR(wrapped1, 0.5f * PI, 0.001f);
    
    float wrapped2 = MathUtils::NormalizeAngle(-2.5f * PI); // -2.5 PI -> -0.5 PI
    EXPECT_NEAR(wrapped2, -0.5f * PI, 0.001f);
    
    TEST_PASS();
}

// Feature 6: Safe Vector Math
void Test_F6_SafeVectorMath() {
    g_currentTestName = "Test_F6_SafeVectorMath";
    Vector2 v = { 3.0f, 4.0f };
    Vector2 norm = MathUtils::SafeNormalize(v);
    EXPECT_NEAR(norm.x, 0.6f, 0.001f);
    EXPECT_NEAR(norm.y, 0.8f, 0.001f);
    EXPECT_NEAR(Vector2Length(norm), 1.0f, 0.001f);

    Vector2 zero = { 0.0f, 0.0f };
    Vector2 zeroNorm = MathUtils::SafeNormalize(zero);
    EXPECT_NEAR(zeroNorm.x, 0.0f, 0.0001f);
    EXPECT_NEAR(zeroNorm.y, 0.0f, 0.0001f);

    Vector2 rotated = MathUtils::RotateVector({ 1.0f, 0.0f }, PI / 2.0f);
    EXPECT_NEAR(rotated.x, 0.0f, 0.001f);
    EXPECT_NEAR(rotated.y, 1.0f, 0.001f);

    TEST_PASS();
}

// Feature 7: Dashed Line Drawing Utility (Safe Zero-length handling)
void Test_F7_DashedLineUtility() {
    g_currentTestName = "Test_F7_DashedLineUtility";
    // Calling with start == end must return early without dividing by zero
    Vector2 p = { 100.0f, 100.0f };
    MathUtils::DrawLineDashed(p, p, 5.0f, 5.0f, WHITE);
    TEST_PASS();
}

// Feature 8: Agent Entity Struct Layout & Defaults
void Test_F8_AgentStructLayout() {
    g_currentTestName = "Test_F8_AgentStructLayout";
    Agent agent;
    agent.id = 42;
    agent.role = Role::NCO;
    agent.position = { 10.0f, 20.0f };
    agent.velocity = { 1.0f, -1.0f };
    agent.targetSlot = { 12.0f, 22.0f };
    agent.angle = 0.5f;
    agent.pikeAngle = 0.6f;
    agent.swingTimer = 1.2f;
    agent.mass = 1.0f;
    agent.maxForce = 180.0f;

    EXPECT_EQ(agent.id, 42);
    EXPECT_TRUE(agent.role == Role::NCO);
    EXPECT_NEAR(agent.mass, 1.0f, 0.001f);
    EXPECT_NEAR(agent.maxForce, 180.0f, 0.001f);
    EXPECT_NEAR(agent.position.x, 10.0f, 0.001f);
    EXPECT_NEAR(agent.targetSlot.y, 22.0f, 0.001f);
    TEST_PASS();
}

// Feature 9: 105-Soldier Formation Initialization
void Test_F9_FormationInitialization() {
    g_currentTestName = "Test_F9_FormationInitialization";
    PikeSquare square(600.0f, 400.0f, 10, 10, 26.0f);
    
    EXPECT_EQ(square.GetSoldiers().size(), 105u);
    EXPECT_EQ(square.GetCols(), 10);
    EXPECT_EQ(square.GetRows(), 10);
    EXPECT_NEAR(square.GetSpacing(), 26.0f, 0.001f);
    EXPECT_NEAR(square.GetPosition().x, 600.0f, 0.001f);
    EXPECT_NEAR(square.GetPosition().y, 400.0f, 0.001f);
    EXPECT_TRUE(square.GetState() == PikeState::PORTE);

    int pikemenCount = 0;
    int ncoCount = 0;
    int leaderCount = 0;

    for (const auto& a : square.GetSoldiers()) {
        if (a.role == Role::PIKEMAN) pikemenCount++;
        else if (a.role == Role::NCO) ncoCount++;
        else if (a.role == Role::LEADER) leaderCount++;
    }

    EXPECT_EQ(pikemenCount, 100);
    EXPECT_EQ(ncoCount, 4);
    EXPECT_EQ(leaderCount, 1);
    TEST_PASS();
}

// Feature 10: Slot Geometry Calculation
void Test_F10_SlotGeometry() {
    g_currentTestName = "Test_F10_SlotGeometry";
    PikeSquare square(600.0f, 400.0f, 10, 10, 26.0f);
    square.UpdateTargetSlots();

    // Front center should be at position + rotated front offset
    Vector2 frontCenter = square.GetFrontCenter();
    // Facing up (-PI/2), front center is directly north of position
    EXPECT_NEAR(frontCenter.x, 600.0f, 0.1f);
    EXPECT_TRUE(frontCenter.y < 400.0f); // Should be higher up on screen (lower Y)

    // Check NCO slots (indices 100..103) are further from center than regular soldiers
    float maxSoldierDist = 0.0f;
    for (int i = 0; i < 100; ++i) {
        float d = Vector2Distance(square.GetSoldiers()[i].targetSlot, square.GetPosition());
        if (d > maxSoldierDist) maxSoldierDist = d;
    }

    for (int i = 100; i < 104; ++i) {
        float ncoDist = Vector2Distance(square.GetSoldiers()[i].targetSlot, square.GetPosition());
        EXPECT_TRUE(ncoDist > maxSoldierDist);
    }

    // Leader slot (index 104) should be behind center (higher Y than position when facing north)
    Vector2 leaderSlot = square.GetSoldiers()[104].targetSlot;
    EXPECT_NEAR(leaderSlot.x, 600.0f, 0.1f);
    EXPECT_TRUE(leaderSlot.y > 400.0f);

    TEST_PASS();
}

// Feature 11: Corner Pivot Wheeling
void Test_F11_CornerPivotWheeling() {
    g_currentTestName = "Test_F11_CornerPivotWheeling";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    float initialAngle = square.GetAngle();
    Vector2 initialPos = square.GetPosition();

    // Wheel right
    square.Wheel(0.05f, true, 0.1f);
    EXPECT_NEAR(square.GetAngle(), initialAngle + 0.05f, 0.001f);
    // Position should change because pivot is corner, not center
    EXPECT_TRUE(Vector2Distance(square.GetPosition(), initialPos) > 0.001f);

    // Wheel left
    square.Wheel(-0.05f, false, 0.1f);
    EXPECT_NEAR(square.GetAngle(), initialAngle, 0.001f);

    TEST_PASS();
}

// Feature 12: 90° In-Place Matrix Turn
void Test_F12_InPlaceMatrixTurn() {
    g_currentTestName = "Test_F12_InPlaceMatrixTurn";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    
    int id_0_0 = square.GetSoldiers()[0].id;
    int id_0_9 = square.GetSoldiers()[9].id;
    int id_9_0 = square.GetSoldiers()[90].id;
    int id_9_9 = square.GetSoldiers()[99].id;

    int id_nco_fl = square.GetSoldiers()[100].id;
    int id_nco_fr = square.GetSoldiers()[101].id;
    int id_nco_bl = square.GetSoldiers()[102].id;
    int id_nco_br = square.GetSoldiers()[103].id;

    // CW Turn (90 deg right)
    square.InPlaceTurn90(true);
    
    // In CW turn:
    // old (0,0) moves to new (90), old (0,9) moves to new (0),
    // old (9,9) moves to new (9), old (9,0) moves to new (99).
    EXPECT_EQ(square.GetSoldiers()[0].id, id_0_9);
    EXPECT_EQ(square.GetSoldiers()[9].id, id_9_9);
    EXPECT_EQ(square.GetSoldiers()[90].id, id_0_0);
    EXPECT_EQ(square.GetSoldiers()[99].id, id_9_0);

    // Verify NCO permutation: FL->FR, FR->BR, BL->FL, BR->BL
    EXPECT_EQ(square.GetSoldiers()[100].id, id_nco_fr);
    EXPECT_EQ(square.GetSoldiers()[101].id, id_nco_br);
    EXPECT_EQ(square.GetSoldiers()[102].id, id_nco_fl);
    EXPECT_EQ(square.GetSoldiers()[103].id, id_nco_bl);

    // CCW Turn (90 deg left) reverses it back
    square.InPlaceTurn90(false);
    EXPECT_EQ(square.GetSoldiers()[0].id, id_0_0);
    EXPECT_EQ(square.GetSoldiers()[9].id, id_0_9);
    EXPECT_EQ(square.GetSoldiers()[90].id, id_9_0);
    EXPECT_EQ(square.GetSoldiers()[99].id, id_9_9);

    EXPECT_EQ(square.GetSoldiers()[100].id, id_nco_fl);
    EXPECT_EQ(square.GetSoldiers()[101].id, id_nco_fr);
    EXPECT_EQ(square.GetSoldiers()[102].id, id_nco_bl);
    EXPECT_EQ(square.GetSoldiers()[103].id, id_nco_br);

    TEST_PASS();
}

// Feature 13: Forward/Backward March (Move)
void Test_F13_ForwardBackwardMarch() {
    g_currentTestName = "Test_F13_ForwardBackwardMarch";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    
    square.Move({ 0.0f, -1.0f }, 0.5f, 80.0f); // Move up by 40 px
    EXPECT_NEAR(square.GetPosition().x, 500.0f, 0.001f);
    EXPECT_NEAR(square.GetPosition().y, 460.0f, 0.001f);

    square.Move({ 1.0f, 0.0f }, 1.0f, 50.0f); // Move right by 50 px
    EXPECT_NEAR(square.GetPosition().x, 550.0f, 0.001f);
    EXPECT_NEAR(square.GetPosition().y, 460.0f, 0.001f);

    TEST_PASS();
}

// Feature 14: Two-Phase Waypoints
void Test_F14_TwoPhaseWaypoints() {
    g_currentTestName = "Test_F14_TwoPhaseWaypoints";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    EXPECT_FALSE(square.HasWaypoint());

    square.SetWaypoint({ 800.0f, 200.0f });
    EXPECT_TRUE(square.HasWaypoint());
    EXPECT_NEAR(square.GetWaypoint().x, 800.0f, 0.001f);
    EXPECT_NEAR(square.GetWaypoint().y, 200.0f, 0.001f);

    square.ClearWaypoint();
    EXPECT_FALSE(square.HasWaypoint());

    TEST_PASS();
}

// Feature 15: NCO Tempo Governor
void Test_F15_NcoTempoGovernor() {
    g_currentTestName = "Test_F15_NcoTempoGovernor";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    
    // Initially all soldiers are on their slots -> tempo factor 1.0
    EXPECT_NEAR(square.GetNcoTempoFactor(), 1.0f, 0.01f);

    // Displace an NCO slightly (< 0.40 * spacing = 10.4 px)
    square.GetSoldiers()[100].position.x += 8.0f;
    EXPECT_NEAR(square.GetNcoTempoFactor(), 1.0f, 0.01f);

    // Displace NCO further (1.2 * spacing = 31.2 px)
    square.GetSoldiers()[100].position.x += 23.2f; // total deviation ~ 31.2 px
    float intermediateTempo = square.GetNcoTempoFactor();
    EXPECT_TRUE(intermediateTempo < 1.0f);
    EXPECT_TRUE(intermediateTempo > 0.15f);

    // Displace NCO past stopLimit (>= 2.0 * spacing = 52.0 px)
    square.GetSoldiers()[100].position.x += 50.0f;
    EXPECT_NEAR(square.GetNcoTempoFactor(), 0.15f, 0.001f);

    TEST_PASS();
}

// Feature 16: Cohesion Metric
void Test_F16_CohesionMetric() {
    g_currentTestName = "Test_F16_CohesionMetric";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    
    // Initially perfect alignment -> cohesion 1.0
    EXPECT_NEAR(square.GetCohesionIndex(), 1.0f, 0.001f);

    // Shift all soldiers slightly
    for (auto& s : square.GetSoldiers()) {
        s.position.x += 10.0f;
    }
    float cohesion = square.GetCohesionIndex();
    EXPECT_TRUE(cohesion < 1.0f);
    EXPECT_TRUE(cohesion > 0.0f);

    // Shift soldiers extremely far (> 1.5 * spacing = 39 px)
    for (auto& s : square.GetSoldiers()) {
        s.position.x += 100.0f;
    }
    EXPECT_NEAR(square.GetCohesionIndex(), 0.0f, 0.001f);

    TEST_PASS();
}

// Feature 17: Flocking & Spring Physics
void Test_F17_FlockingAndSpringPhysics() {
    g_currentTestName = "Test_F17_FlockingAndSpringPhysics";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    
    // Displace soldier 0 from slot
    square.GetSoldiers()[0].position.x -= 20.0f;
    Vector2 initialPos = square.GetSoldiers()[0].position;

    // Run one update tick
    square.Update(0.016f);

    // Velocity should be directed toward target slot (positive x)
    EXPECT_TRUE(square.GetSoldiers()[0].velocity.x > 0.0f);
    // Position should have moved toward target slot
    EXPECT_TRUE(square.GetSoldiers()[0].position.x > initialPos.x);

    TEST_PASS();
}

// Feature 18: Dynamic Pike Sway
void Test_F18_DynamicPikeSway() {
    g_currentTestName = "Test_F18_DynamicPikeSway";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    
    // In PORTE state
    square.pikeState = PikeState::PORTE;
    square.GetSoldiers()[0].velocity = { 50.0f, 0.0f };
    float initialTimer = square.GetSoldiers()[0].swingTimer;
    
    square.Update(0.016f);
    
    // Swing timer must increase when agent speed > 5.0f
    EXPECT_TRUE(square.GetSoldiers()[0].swingTimer > initialTimer);

    TEST_PASS();
}

// Feature 19: IRenderer Interface
class MockHeadlessRenderer : public IRenderer {
public:
    int sceneDrawn = 0;
    int bgDrawn = 0;
    int formationDrawn = 0;
    int agentDrawn = 0;
    int waypointDrawn = 0;

    void DrawScene(const PikeSquare& square) override {
        sceneDrawn++;
        DrawBackground();
        DrawFormation(square);
        if (square.HasWaypoint()) {
            DrawWaypoint(square.GetWaypoint(), square.GetFrontCenter());
        }
    }
    void DrawBackground() override { bgDrawn++; }
    void DrawFormation(const PikeSquare& square) override {
        formationDrawn++;
        for (const auto& a : square.GetSoldiers()) {
            DrawAgent(a, square.GetSpacing(), square.GetState());
        }
    }
    void DrawAgent(const Agent& /*agent*/, float /*spacing*/, PikeState /*state*/) override {
        agentDrawn++;
    }
    void DrawWaypoint(Vector2 /*targetPos*/, Vector2 /*frontCenter*/) override {
        waypointDrawn++;
    }
};

void Test_F19_IRendererInterface() {
    g_currentTestName = "Test_F19_IRendererInterface";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    square.SetWaypoint({ 600.0f, 300.0f });

    std::unique_ptr<IRenderer> renderer = std::make_unique<MockHeadlessRenderer>();
    renderer->Render(square);

    auto* mock = dynamic_cast<MockHeadlessRenderer*>(renderer.get());
    EXPECT_TRUE(mock != nullptr);
    EXPECT_EQ(mock->sceneDrawn, 1);
    EXPECT_EQ(mock->bgDrawn, 1);
    EXPECT_EQ(mock->formationDrawn, 1);
    EXPECT_EQ(mock->agentDrawn, 105);
    EXPECT_EQ(mock->waypointDrawn, 1);

    TEST_PASS();
}

// Feature 20: Engraving Renderer Subsystem
void Test_F20_EngravingRendererSubsystem() {
    g_currentTestName = "Test_F20_EngravingRendererSubsystem";
    std::unique_ptr<IRenderer> renderer = std::make_unique<EngravingRenderer>();
    EXPECT_TRUE(renderer != nullptr);
    EXPECT_TRUE(dynamic_cast<EngravingRenderer*>(renderer.get()) != nullptr);
    TEST_PASS();
}

// Feature 21: Vector Renderer Subsystem
void Test_F21_VectorRendererSubsystem() {
    g_currentTestName = "Test_F21_VectorRendererSubsystem";
    std::unique_ptr<IRenderer> renderer = std::make_unique<VectorRenderer>();
    EXPECT_TRUE(renderer != nullptr);
    EXPECT_TRUE(dynamic_cast<VectorRenderer*>(renderer.get()) != nullptr);
    TEST_PASS();
}

// Feature 22: UIManager HUD Interface
void Test_F22_UIManagerInterface() {
    g_currentTestName = "Test_F22_UIManagerInterface";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    
    // Verify telemetry data accessed by HUD is robust and non-empty
    EXPECT_EQ(square.GetSoldiers().size(), 105u);
    EXPECT_NEAR(square.GetNcoTempoFactor(), 1.0f, 0.01f);
    EXPECT_NEAR(square.GetCohesionIndex(), 1.0f, 0.01f);
    EXPECT_TRUE(square.GetState() == PikeState::PORTE);

    TEST_PASS();
}

// Feature 23: Streamlined main.cpp Structure (<100 lines, cleanly decoupled)
void Test_F23_StreamlinedMainStructure() {
    g_currentTestName = "Test_F23_StreamlinedMainStructure";
    std::ifstream file("src/main.cpp");
    EXPECT_TRUE(file.is_open());
    
    std::string line;
    int lineCount = 0;
    bool foundPikeSquareClass = false;
    while (std::getline(file, line)) {
        lineCount++;
        if (line.find("class PikeSquare") != std::string::npos) {
            foundPikeSquareClass = true;
        }
    }
    
    EXPECT_TRUE(lineCount < 100);
    EXPECT_FALSE(foundPikeSquareClass); // Must be in separate module
    TEST_PASS();
}

// Feature 24: Modular CMake Configuration
void Test_F24_ModularCMakeConfiguration() {
    g_currentTestName = "Test_F24_ModularCMakeConfiguration";
    std::ifstream file("CMakeLists.txt");
    EXPECT_TRUE(file.is_open());

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("target_include_directories") != std::string::npos);
    EXPECT_TRUE(content.find("src/formation/PikeSquare.cpp") != std::string::npos);
    EXPECT_TRUE(content.find("src/rendering/EngravingRenderer.cpp") != std::string::npos);
    TEST_PASS();
}

// ============================================================================
// TIER 2: BOUNDARY & CORNER CASES
// ============================================================================

// T2: Zero Length Vector Safe Normalization
void Test_T2_ZeroLengthVector() {
    g_currentTestName = "Test_T2_ZeroLengthVector";
    Vector2 zero = { 0.0f, 0.0f };
    Vector2 res = MathUtils::SafeNormalize(zero);
    EXPECT_NEAR(res.x, 0.0f, 0.00001f);
    EXPECT_NEAR(res.y, 0.0f, 0.00001f);
    TEST_PASS();
}

// T2: Sub-Epsilon Vector Safe Normalization
void Test_T2_SubEpsilonVector() {
    g_currentTestName = "Test_T2_SubEpsilonVector";
    Vector2 tiny = { 1e-8f, 1e-8f };
    Vector2 res = MathUtils::SafeNormalize(tiny, 0.0001f);
    EXPECT_NEAR(res.x, 0.0f, 0.00001f);
    EXPECT_NEAR(res.y, 0.0f, 0.00001f);
    TEST_PASS();
}

// T2: Extreme Positive Angle Normalization
void Test_T2_ExtremePositiveAngle() {
    g_currentTestName = "Test_T2_ExtremePositiveAngle";
    float extreme = 100.0f * PI + 0.5f;
    float norm = MathUtils::NormalizeAngle(extreme);
    EXPECT_TRUE(norm >= -PI && norm <= PI);
    EXPECT_NEAR(norm, 0.5f, 0.01f);
    TEST_PASS();
}

// T2: Extreme Negative Angle Normalization
void Test_T2_ExtremeNegativeAngle() {
    g_currentTestName = "Test_T2_ExtremeNegativeAngle";
    float extreme = -100.0f * PI - 0.75f;
    float norm = MathUtils::NormalizeAngle(extreme);
    EXPECT_TRUE(norm >= -PI && norm <= PI);
    EXPECT_NEAR(norm, -0.75f, 0.01f);
    TEST_PASS();
}

// T2: Exact Boundary Angles
void Test_T2_ExactBoundaryAngles() {
    g_currentTestName = "Test_T2_ExactBoundaryAngles";
    float n1 = MathUtils::NormalizeAngle(PI);
    EXPECT_TRUE(n1 >= -PI && n1 <= PI);
    float n2 = MathUtils::NormalizeAngle(-PI);
    EXPECT_TRUE(n2 >= -PI && n2 <= PI);
    TEST_PASS();
}

// T2: Zero Delta Time Update
void Test_T2_ZeroDeltaTime() {
    g_currentTestName = "Test_T2_ZeroDeltaTime";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    square.Update(0.0f);
    for (const auto& a : square.GetSoldiers()) {
        EXPECT_FALSE(std::isnan(a.position.x));
        EXPECT_FALSE(std::isnan(a.position.y));
        EXPECT_FALSE(std::isnan(a.velocity.x));
        EXPECT_FALSE(std::isnan(a.velocity.y));
    }
    TEST_PASS();
}

// T2: Large Delta Time Numerical Stability
void Test_T2_LargeDeltaTime() {
    g_currentTestName = "Test_T2_LargeDeltaTime";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    square.GetSoldiers()[0].position.x += 100.0f;
    square.Update(2.0f); // 2 full seconds in one frame
    for (const auto& a : square.GetSoldiers()) {
        EXPECT_FALSE(std::isnan(a.position.x));
        EXPECT_FALSE(std::isnan(a.position.y));
        EXPECT_FALSE(std::isnan(a.velocity.x));
        EXPECT_FALSE(std::isnan(a.velocity.y));
        EXPECT_FALSE(std::isinf(a.position.x));
        EXPECT_FALSE(std::isinf(a.velocity.x));
    }
    TEST_PASS();
}

// T2: NCO Extreme Displacement Bound
void Test_T2_NcoExtremeDisplacementBound() {
    g_currentTestName = "Test_T2_NcoExtremeDisplacementBound";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    // Move NCO far away
    square.GetSoldiers()[100].position = { 5000.0f, 5000.0f };
    float factor = square.GetNcoTempoFactor();
    EXPECT_NEAR(factor, 0.15f, 0.0001f);
    EXPECT_TRUE(factor >= 0.15f);
    TEST_PASS();
}

// T2: Cohesion Lower Bound (Clamped at Zero)
void Test_T2_CohesionLowerBound() {
    g_currentTestName = "Test_T2_CohesionLowerBound";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    for (auto& s : square.GetSoldiers()) {
        s.position = { 100000.0f, 100000.0f };
    }
    float cohesion = square.GetCohesionIndex();
    EXPECT_NEAR(cohesion, 0.0f, 0.00001f);
    EXPECT_TRUE(cohesion >= 0.0f);
    TEST_PASS();
}

// T2: Speed Clamp Enforcement Under Extreme Impulses
void Test_T2_SpeedClampEnforcement() {
    g_currentTestName = "Test_T2_SpeedClampEnforcement";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    square.GetSoldiers()[0].velocity = { 99999.0f, 99999.0f };
    
    square.Update(0.016f);
    
    float velMag = Vector2Length(square.GetSoldiers()[0].velocity);
    // Base speed in PORTE is 80.0f * tempoFactor(1.0f) = 80.0f.
    // maxAllowedSpeed = speed * 1.5 + rotationSpeed * 160 = 120 + 128 = 248.
    // Damped by 0.85 -> should be well under 300.0f
    EXPECT_TRUE(velMag < 300.0f);
    TEST_PASS();
}

// T2: Waypoint At Exactly Front Center
void Test_T2_WaypointAtFrontCenter() {
    g_currentTestName = "Test_T2_WaypointAtFrontCenter";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    Vector2 front = square.GetFrontCenter();
    square.SetWaypoint(front);
    EXPECT_TRUE(square.HasWaypoint());
    
    square.Update(0.016f);
    // Since distance <= 5.0f, waypoint should be arrived at and cleared
    EXPECT_FALSE(square.HasWaypoint());
    TEST_PASS();
}

// ============================================================================
// TIER 3: CROSS-FEATURE COMBINATIONS
// ============================================================================

// T3: 90° Turn During Active Waypoint Clears Waypoint
void Test_T3_TurnDuringWaypointClearsWaypoint() {
    g_currentTestName = "Test_T3_TurnDuringWaypointClearsWaypoint";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    square.SetWaypoint({ 800.0f, 300.0f });
    EXPECT_TRUE(square.HasWaypoint());

    square.InPlaceTurn90(true);
    // Shift+A/D in-place turn must clear active waypoint per spec
    EXPECT_FALSE(square.HasWaypoint());
    TEST_PASS();
}

// T3: NCO Tempo Throttling Scales Formation Speed
void Test_T3_TempoThrottlingScalesSpeed() {
    g_currentTestName = "Test_T3_TempoThrottlingScalesSpeed";
    PikeSquare squareFullTempo(500.0f, 500.0f, 10, 10, 26.0f);
    PikeSquare squareSlowTempo(500.0f, 500.0f, 10, 10, 26.0f);

    // Displace NCO in second square so tempo factor drops to 0.15
    squareSlowTempo.GetSoldiers()[100].position.x += 100.0f;
    EXPECT_NEAR(squareSlowTempo.GetNcoTempoFactor(), 0.15f, 0.001f);

    // Both navigate towards distant waypoint
    Vector2 wp = { 500.0f, 100.0f };
    squareFullTempo.SetWaypoint(wp);
    squareSlowTempo.SetWaypoint(wp);

    squareFullTempo.Update(0.1f);
    squareSlowTempo.Update(0.1f);

    float moveDistFull = Vector2Distance(squareFullTempo.GetPosition(), { 500.0f, 500.0f });
    float moveDistSlow = Vector2Distance(squareSlowTempo.GetPosition(), { 500.0f, 500.0f });

    // Slow tempo should cover significantly less distance
    EXPECT_TRUE(moveDistFull > moveDistSlow);
    TEST_PASS();
}

// T3: Posture Transition Affects Speed and Sway
void Test_T3_PostureTransition() {
    g_currentTestName = "Test_T3_PostureTransition";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    EXPECT_TRUE(square.pikeState == PikeState::PORTE);

    // Transition to CHARGE
    square.pikeState = PikeState::CHARGE;
    EXPECT_TRUE(square.GetState() == PikeState::CHARGE);

    // Update in CHARGE
    square.GetSoldiers()[0].velocity = { 20.0f, 0.0f };
    square.Update(0.016f);

    // In CHARGE, pike angle sway uses amplitude 0.12f
    EXPECT_TRUE(square.GetSoldiers()[0].pikeAngle != square.GetSoldiers()[0].angle);
    TEST_PASS();
}

// T3: Waypoint Multi-step Navigation (Phase 1 Alignment to Phase 2 March)
void Test_T3_WaypointTwoPhaseNavigation() {
    g_currentTestName = "Test_T3_WaypointTwoPhaseNavigation";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    
    // Set waypoint 45 degrees to the right
    Vector2 wp = { 650.0f, 250.0f };
    square.SetWaypoint(wp);

    // Initially, formation is facing North (-PI/2)
    Vector2 front = square.GetFrontCenter();
    Vector2 toWp = Vector2Subtract(wp, front);
    float targetAngle = atan2f(toWp.y, toWp.x);
    float angleDiff = MathUtils::NormalizeAngle(targetAngle - square.GetAngle());
    EXPECT_TRUE(fabsf(angleDiff) > 0.03f); // Must be in Phase 1 (turning)

    // Step simulation until alignment occurs
    int maxSteps = 400;
    while (square.HasWaypoint() && maxSteps-- > 0) {
        square.Update(0.05f);
        Vector2 curFront = square.GetFrontCenter();
        Vector2 curToWp = Vector2Subtract(wp, curFront);
        float curTargetAngle = atan2f(curToWp.y, curToWp.x);
        float curDiff = MathUtils::NormalizeAngle(curTargetAngle - square.GetAngle());
        if (fabsf(curDiff) < 0.03f) {
            // Reached Phase 2!
            break;
        }
    }

    EXPECT_TRUE(maxSteps > 0); // Alignment succeeded within budget
    TEST_PASS();
}

// ============================================================================
// TIER 4: REAL-WORLD SCENARIOS (Full Tactical Drills)
// ============================================================================

// T4: Full Tactical Renaissance Drill Sequence & Cohesion Recovery
void Test_T4_TacticalDrillSequenceAndCohesionRecovery() {
    g_currentTestName = "Test_T4_TacticalDrillSequenceAndCohesionRecovery";
    PikeSquare square(600.0f, 400.0f, 10, 10, 26.0f);

    // Phase 1: Formation integrity check
    EXPECT_EQ(square.GetSoldiers().size(), 105u);
    EXPECT_NEAR(square.GetCohesionIndex(), 1.0f, 0.001f);
    EXPECT_NEAR(square.GetNcoTempoFactor(), 1.0f, 0.001f);

    // Phase 2: Forward March simulation
    Vector2 startPos = square.GetPosition();
    for (int i = 0; i < 40; ++i) {
        square.Move({ 0.0f, -1.0f }, 0.016f, 80.0f);
        square.Update(0.016f);
    }
    EXPECT_TRUE(square.GetPosition().y < startPos.y);

    // Upon halting, ranks reform and dress lines over relaxation ticks
    for (int i = 0; i < 60; ++i) {
        square.Update(0.016f);
    }
    // During active translation and initial recovery, soldiers trail slots with cohesion bounded above 0.20
    EXPECT_TRUE(square.GetCohesionIndex() > 0.20f);

    // Once halted for settling (500 frames), cohesion settles to > 0.90
    for (int i = 60; i < 500; ++i) {
        square.Update(0.016f);
    }
    EXPECT_TRUE(square.GetCohesionIndex() > 0.90f);

    // Phase 3: Corner Wheel maneuver (rotate 30 degrees)
    float prevAngle = square.GetAngle();
    for (int i = 0; i < 30; ++i) {
        square.Wheel(0.02f, true, 0.016f);
        square.Update(0.016f);
    }
    EXPECT_TRUE(square.GetAngle() > prevAngle);

    // Dress ranks after wheeling maneuver (settling)
    for (int i = 0; i < 500; ++i) {
        square.Update(0.016f);
    }
    EXPECT_TRUE(square.GetCohesionIndex() > 0.60f);

    // Phase 4: In-Place Face Turn (soldiers permute slots)
    square.InPlaceTurn90(true);
    EXPECT_TRUE(square.GetCohesionIndex() > 0.60f);

    // Let the square settle further to crisp formation (> 0.90)
    for (int i = 0; i < 500; ++i) {
        square.Update(0.016f);
    }
    EXPECT_TRUE(square.GetCohesionIndex() > 0.90f);

    // Phase 5: Cohesion perturbation & relaxation stabilization
    // Simulate severe artillery disruption by scattering soldiers deterministically up to 12 px
    for (size_t i = 0; i < square.GetSoldiers().size(); ++i) {
        float dx = static_cast<float>(static_cast<int>(i % 7) - 3) * 4.0f;
        float dy = static_cast<float>(static_cast<int>(i % 5) - 2) * 4.0f;
        square.GetSoldiers()[i].position.x += dx;
        square.GetSoldiers()[i].position.y += dy;
    }
    
    float perturbedCohesion = square.GetCohesionIndex();
    EXPECT_TRUE(perturbedCohesion < 0.88f);

    // Run relaxation frames to absorb shock and reform ranks
    for (int i = 0; i < 400; ++i) {
        square.Update(0.016f);
    }

    // Cohesion must have recovered significantly (> 0.90 and greater than perturbed)
    float recoveredCohesion = square.GetCohesionIndex();
    EXPECT_TRUE(recoveredCohesion > perturbedCohesion);
    EXPECT_TRUE(recoveredCohesion > 0.90f);

    TEST_PASS();
}

// ============================================================================
// MAIN RUNNER
// ============================================================================

int main() {
    std::cout << "=========================================================\n";
    std::cout << "       PIKESIM EXHAUSTIVE HEADLESS TEST SUITE\n";
    std::cout << "   Tiers 1-4: Feature, Boundary, Cross & Tactical Drills\n";
    std::cout << "=========================================================\n\n";

    std::cout << "--- TIER 1: FEATURE COVERAGE ---\n";
    Test_F1_ScreenConstants();
    Test_F2_RoleEnumeration();
    Test_F3_PikeStateEnumeration();
    Test_F4_VisualModeEnumeration();
    Test_F5_AngleNormalization();
    Test_F6_SafeVectorMath();
    Test_F7_DashedLineUtility();
    Test_F8_AgentStructLayout();
    Test_F9_FormationInitialization();
    Test_F10_SlotGeometry();
    Test_F11_CornerPivotWheeling();
    Test_F12_InPlaceMatrixTurn();
    Test_F13_ForwardBackwardMarch();
    Test_F14_TwoPhaseWaypoints();
    Test_F15_NcoTempoGovernor();
    Test_F16_CohesionMetric();
    Test_F17_FlockingAndSpringPhysics();
    Test_F18_DynamicPikeSway();
    Test_F19_IRendererInterface();
    Test_F20_EngravingRendererSubsystem();
    Test_F21_VectorRendererSubsystem();
    Test_F22_UIManagerInterface();
    Test_F23_StreamlinedMainStructure();
    Test_F24_ModularCMakeConfiguration();

    std::cout << "\n--- TIER 2: BOUNDARY & CORNER CASES ---\n";
    Test_T2_ZeroLengthVector();
    Test_T2_SubEpsilonVector();
    Test_T2_ExtremePositiveAngle();
    Test_T2_ExtremeNegativeAngle();
    Test_T2_ExactBoundaryAngles();
    Test_T2_ZeroDeltaTime();
    Test_T2_LargeDeltaTime();
    Test_T2_NcoExtremeDisplacementBound();
    Test_T2_CohesionLowerBound();
    Test_T2_SpeedClampEnforcement();
    Test_T2_WaypointAtFrontCenter();

    std::cout << "\n--- TIER 3: CROSS-FEATURE COMBINATIONS ---\n";
    Test_T3_TurnDuringWaypointClearsWaypoint();
    Test_T3_TempoThrottlingScalesSpeed();
    Test_T3_PostureTransition();
    Test_T3_WaypointTwoPhaseNavigation();

    std::cout << "\n--- TIER 4: REAL-WORLD SCENARIOS (TACTICAL DRILLS) ---\n";
    Test_T4_TacticalDrillSequenceAndCohesionRecovery();

    std::cout << "\n=========================================================\n";
    std::cout << "TEST RESULTS SUMMARY:\n";
    std::cout << "  Total Passed: " << g_testsPassed << "\n";
    std::cout << "  Total Failed: " << g_testsFailed << "\n";
    std::cout << "=========================================================\n";

    if (g_testsFailed == 0) {
        std::cout << "\n>>> ALL TESTS PASSED SUCCESSFULLY! <<<\n";
        return 0;
    } else {
        std::cerr << "\n>>> TEST FAILURES DETECTED! <<<\n";
        return 1;
    }
}
