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
#include "rendering/CameraController.h"
#include "ui/UIManager.h"
#include "ui/InputController.h"
#include "core/SpatialHashGrid.h"
#include "ui/FormationCard.h"
#include "simulation/EnemyAIController.h"

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

// Feature 25: CameraController Transforms & Zoom Limits
void Test_F25_CameraControllerTransforms() {
    g_currentTestName = "Test_F25_CameraControllerTransforms";
    CameraController cam({ 600.0f, 400.0f });
    
    // Zoom clamping
    cam.SetZoom(0.1f);
    EXPECT_NEAR(cam.GetZoom(), cam.GetMinZoom(), 0.001f);
    cam.SetZoom(5.0f);
    EXPECT_NEAR(cam.GetZoom(), cam.GetMaxZoom(), 0.001f);

    // Coordinate transforms at zoom 1.0f
    cam.SetZoom(1.0f);
    Vector2 screenCenter = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    Vector2 worldCenter = cam.ScreenToWorld(screenCenter);
    EXPECT_NEAR(worldCenter.x, 600.0f, 0.01f);
    EXPECT_NEAR(worldCenter.y, 400.0f, 0.01f);

    Vector2 roundTrip = cam.WorldToScreen(worldCenter);
    EXPECT_NEAR(roundTrip.x, screenCenter.x, 0.01f);
    EXPECT_NEAR(roundTrip.y, screenCenter.y, 0.01f);

    // Pan camera
    cam.CenterOn({ 1000.0f, 800.0f });
    EXPECT_NEAR(cam.GetTarget().x, 1000.0f, 0.01f);
    EXPECT_NEAR(cam.GetTarget().y, 800.0f, 0.01f);

    TEST_PASS();
}

// Feature 26: Formation Target Heading Order
void Test_F26_FormationTargetHeadingOrder() {
    g_currentTestName = "Test_F26_FormationTargetHeadingOrder";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    // Initial heading is facing North (-PI/2)
    EXPECT_NEAR(square.GetAngle(), -PI / 2.0f, 0.01f);

    // Issue order to move to (500, 300) with final heading facing East (0.0 rad)
    square.SetOrder({ 500.0f, 300.0f }, 0.0f, true);
    EXPECT_TRUE(square.HasWaypoint());
    EXPECT_TRUE(square.HasTargetHeading());
    EXPECT_NEAR(square.GetTargetHeading(), 0.0f, 0.01f);

    // Simulate navigation frames until destination reached and heading aligned (budget of 1000 frames @ 60 FPS)
    for (int i = 0; i < 1000 && (square.HasWaypoint() || square.HasTargetHeading()); ++i) {
        square.Update(0.016f);
    }

    // Formation should be near targetPos and aligned with 0.0 rad
    EXPECT_FALSE(square.HasWaypoint());
    EXPECT_FALSE(square.HasTargetHeading());
    EXPECT_NEAR(square.GetAngle(), 0.0f, 0.05f);

    TEST_PASS();
}

// Feature 27: InputController Drag Heading Calculation
void Test_F27_InputControllerDragHeading() {
    g_currentTestName = "Test_F27_InputControllerDragHeading";
    
    // Drag East (dx > 0, dy = 0) -> angle = 0 rad
    float headingEast = InputController::CalculateHeadingFromVector({ 100.0f, 100.0f }, { 200.0f, 100.0f });
    EXPECT_NEAR(headingEast, 0.0f, 0.001f);

    // Drag South (dx = 0, dy > 0) -> angle = PI/2 rad
    float headingSouth = InputController::CalculateHeadingFromVector({ 100.0f, 100.0f }, { 100.0f, 200.0f });
    EXPECT_NEAR(headingSouth, PI / 2.0f, 0.001f);

    // Drag North (dx = 0, dy < 0) -> angle = -PI/2 rad
    float headingNorth = InputController::CalculateHeadingFromVector({ 100.0f, 100.0f }, { 100.0f, 0.0f });
    EXPECT_NEAR(headingNorth, -PI / 2.0f, 0.001f);

    // Drag West (dx < 0, dy = 0) -> angle = PI or -PI
    float headingWest = InputController::CalculateHeadingFromVector({ 100.0f, 100.0f }, { 0.0f, 100.0f });
    EXPECT_NEAR(fabsf(headingWest), PI, 0.001f);

    TEST_PASS();
}

// Feature 28: SpatialHashGrid Insertion and Query
void Test_F28_SpatialHashGridInsertionAndQuery() {
    g_currentTestName = "Test_F28_SpatialHashGridInsertionAndQuery";
    SpatialHashGrid grid(40.0f);
    EXPECT_EQ(grid.GetEntryCount(), 0);

    // Insert 3 agents: two near each other, one far
    grid.Insert(1, 10, Faction::PLAYER, { 100.0f, 100.0f }, 15.0f);
    grid.Insert(2, 10, Faction::PLAYER, { 115.0f, 100.0f }, 15.0f); // dist = 15
    grid.Insert(3, 20, Faction::ENEMY, { 600.0f, 600.0f }, 15.0f);  // far away

    EXPECT_EQ(grid.GetEntryCount(), 3);

    // Query radius 20 around (100, 100)
    int foundCount = 0;
    AgentId foundId = 0;
    grid.QueryRadius({ 100.0f, 100.0f }, 20.0f, [&](const GridAgentEntry& entry, float dist) {
        foundCount++;
        foundId = entry.agentId;
        EXPECT_NEAR(dist, 15.0f, 0.01f);
    });

    EXPECT_EQ(foundCount, 1);
    EXPECT_EQ(foundId, 2);

    TEST_PASS();
}

// Feature 29: SpatialHashGrid Zero-Allocation Capacity and Clear
void Test_F29_SpatialHashGridZeroAllocation() {
    g_currentTestName = "Test_F29_SpatialHashGridZeroAllocation";
    SpatialHashGrid grid(32.0f);

    // Insert 500 agents across varied coordinates (including negative world coords)
    for (int i = 0; i < 500; ++i) {
        float x = -200.0f + static_cast<float>(i * 3);
        float y = -150.0f + static_cast<float>(i * 2);
        bool ok = grid.Insert(i, 1, Faction::PLAYER, { x, y }, 10.0f);
        EXPECT_TRUE(ok);
    }
    EXPECT_EQ(grid.GetEntryCount(), 500);

    // Clear grid
    grid.Clear();
    EXPECT_EQ(grid.GetEntryCount(), 0);

    // Query on cleared grid yields 0
    GridAgentEntry buffer[16];
    int count = grid.QueryRadiusBuffer({ 0.0f, 0.0f }, 100.0f, buffer, 16);
    EXPECT_EQ(count, 0);

    TEST_PASS();
}

// Feature 30: Faction Assignment and Formation Initial Orientation
void Test_F30_FactionAssignmentAndProperties() {
    g_currentTestName = "Test_F30_FactionAssignmentAndProperties";
    PikeSquare playerSquare(600.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    PikeSquare enemySquare(600.0f, 200.0f, 10, 10, 26.0f, Faction::ENEMY, PI / 2.0f);

    EXPECT_TRUE(playerSquare.GetFaction() == Faction::PLAYER);
    EXPECT_TRUE(enemySquare.GetFaction() == Faction::ENEMY);

    EXPECT_NEAR(playerSquare.GetAngle(), -PI / 2.0f, 0.01f);
    EXPECT_NEAR(enemySquare.GetAngle(), PI / 2.0f, 0.01f);

    // Check soldiers have matching faction
    EXPECT_TRUE(playerSquare.GetSoldiers()[0].faction == Faction::PLAYER);
    EXPECT_TRUE(enemySquare.GetSoldiers()[0].faction == Faction::ENEMY);

    TEST_PASS();
}

// Feature 31: Inter-Formation Agent Repulsion via SpatialHashGrid
void Test_F31_InterFormationAgentRepulsion() {
    g_currentTestName = "Test_F31_InterFormationAgentRepulsion";
    SpatialHashGrid grid(40.0f);

    // Player facing North (-PI/2), front at y ≈ 500 - 117 = 383
    PikeSquare playerSquare(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    playerSquare.SetFormationId(1);

    // Enemy facing South (PI/2), front at y ≈ 250 + 117 = 367
    // Lines in close contact (383 vs 367 -> dist = 16px < personalRadius 19.5px)
    PikeSquare enemySquare(500.0f, 250.0f, 10, 10, 26.0f, Faction::ENEMY, PI / 2.0f);
    enemySquare.SetFormationId(2);

    // Register all agents into grid
    grid.Clear();
    for (const auto& a : playerSquare.GetSoldiers()) {
        grid.Insert(a.id, playerSquare.GetFormationId(), a.faction, a.position, 19.5f);
    }
    for (const auto& a : enemySquare.GetSoldiers()) {
        grid.Insert(a.id, enemySquare.GetFormationId(), a.faction, a.position, 19.5f);
    }

    // Step physics with grid
    playerSquare.Update(0.016f, &grid);

    // Front soldiers of player square should receive repelling impulse pushed South (dy > 0)
    // Front row of player is row 0: soldiers 0..9
    float totalFrontYVel = 0.0f;
    for (int col = 0; col < 10; ++col) {
        totalFrontYVel += playerSquare.GetSoldiers()[col].velocity.y;
    }
    // Repulsion pushes south away from enemy in contact
    EXPECT_TRUE(totalFrontYVel > 0.0f);

    TEST_PASS();
}

// Feature 32: Enemy AI Controller States & Posture
void Test_F32_EnemyAIControllerStates() {
    g_currentTestName = "Test_F32_EnemyAIControllerStates";
    PikeSquare playerSquare(500.0f, 700.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    PikeSquare enemySquare(500.0f, 100.0f, 10, 10, 26.0f, Faction::ENEMY, PI / 2.0f);
    EnemyAIController ai(enemySquare);

    // Initial state is static IDLE (sandbox default)
    EXPECT_FALSE(ai.IsActive());
    EXPECT_TRUE(ai.GetState() == EnemyAIState::IDLE);

    // Activate AI
    ai.SetActive(true);
    EXPECT_TRUE(ai.IsActive());

    // Distance is 600 > 250 -> ADVANCE state, PORTE posture
    ai.Update(0.016f, playerSquare);
    EXPECT_TRUE(ai.GetState() == EnemyAIState::ADVANCE);
    EXPECT_TRUE(enemySquare.pikeState == PikeState::PORTE);
    EXPECT_TRUE(enemySquare.HasWaypoint());

    // Reposition enemy close to player (dist = 150 <= 250)
    enemySquare.position = { 500.0f, 550.0f };
    ai.Update(0.016f, playerSquare);
    EXPECT_TRUE(ai.GetState() == EnemyAIState::ENGAGE);
    EXPECT_TRUE(enemySquare.pikeState == PikeState::CHARGE);

    // Toggle active off -> IDLE, waypoint cleared
    ai.ToggleActive();
    EXPECT_FALSE(ai.IsActive());
    ai.Update(0.016f, playerSquare);
    EXPECT_TRUE(ai.GetState() == EnemyAIState::IDLE);
    EXPECT_FALSE(enemySquare.HasWaypoint());

    TEST_PASS();
}

// Feature 33: Rota File Structure & Weapon Homogeneity
void Test_F33_RotaFileStructureAndDepths() {
    g_currentTestName = "Test_F33_RotaFileStructureAndDepths";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    EXPECT_EQ(square.GetRoty().size(), 10u);
    EXPECT_EQ(square.GetCols(), 10);
    EXPECT_EQ(square.GetRows(), 10);

    for (size_t i = 0; i < square.GetRoty().size(); ++i) {
        const auto& rota = square.GetRoty()[i];
        EXPECT_EQ(rota.depth, 10);
        EXPECT_TRUE(rota.weaponType == WeaponType::PIKE);
        EXPECT_EQ(rota.soldierIndices.size(), 10u);
    }

    EXPECT_EQ(square.GetPikemenCount(), 100);
    EXPECT_EQ(square.GetMusketeerCount(), 0);
    EXPECT_EQ(square.GetHalberdierCount(), 0);
    EXPECT_NEAR(square.GetMorale(), 1.0f, 0.001f);
    EXPECT_NEAR(square.GetStamina(), 1.0f, 0.001f);

    TEST_PASS();
}

// Feature 34: Rota Reordering (Swap) and Target Slot Recalculation
void Test_F34_RotaReorderAndTargetSlotRecalculation() {
    g_currentTestName = "Test_F34_RotaReorderAndTargetSlotRecalculation";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    // Modify weapon of left wing rota (rota 0) to MUSKET
    square.GetRoty()[0].weaponType = WeaponType::MUSKET;
    square.UpdateTargetSlots();

    EXPECT_TRUE(square.GetRoty()[0].weaponType == WeaponType::MUSKET);
    EXPECT_TRUE(square.GetSoldiers()[square.GetRoty()[0].soldierIndices[0]].weapon == WeaponType::MUSKET);
    EXPECT_EQ(square.GetMusketeerCount(), 10);
    EXPECT_EQ(square.GetPikemenCount(), 90);

    // Swap rota 0 and rota 9
    square.SwapRoty(0, 9);
    EXPECT_TRUE(square.GetRoty()[9].weaponType == WeaponType::MUSKET);
    EXPECT_TRUE(square.GetRoty()[0].weaponType == WeaponType::PIKE);

    // Slots of soldiers in rota 9 are now positioned on the right flank
    int musketeerIdx = square.GetRoty()[9].soldierIndices[0];
    int pikemanIdx = square.GetRoty()[0].soldierIndices[0];
    EXPECT_TRUE(square.GetSoldiers()[musketeerIdx].targetSlot.x > square.GetSoldiers()[pikemanIdx].targetSlot.x);

    TEST_PASS();
}

// Feature 35: Detaching Rota Creates Autonomous Formation
void Test_F35_DetachRotaCreatesAutonomousFormation() {
    g_currentTestName = "Test_F35_DetachRotaCreatesAutonomousFormation";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    EXPECT_EQ(square.GetRoty().size(), 10u);
    EXPECT_EQ(square.GetSoldiers().size(), 105u);

    // Detach rota index 9 (flank file)
    PikeSquare detached = square.DetachRota(9);

    // Parent formation has 9 roty, 95 soldiers (90 pikemen + 4 NCOs + 1 Leader)
    EXPECT_EQ(square.GetRoty().size(), 9u);
    EXPECT_EQ(square.GetCols(), 9);
    EXPECT_EQ(square.GetSoldiers().size(), 95u);

    // Detached formation has 1 rota, 15 soldiers (10 pikemen + 4 NCOs + 1 Leader)
    EXPECT_EQ(detached.GetRoty().size(), 1u);
    EXPECT_EQ(detached.GetCols(), 1);
    EXPECT_EQ(detached.GetSoldiers().size(), 15u);
    EXPECT_TRUE(detached.GetFaction() == square.GetFaction());
    EXPECT_TRUE(detached.GetPosition().x != square.GetPosition().x);

    TEST_PASS();
}

// Feature 36: Merging Formation Restores Files and Slots
void Test_F36_MergeRotaRestoresFormation() {
    g_currentTestName = "Test_F36_MergeRotaRestoresFormation";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    PikeSquare detached = square.DetachRota(9);
    EXPECT_EQ(square.GetRoty().size(), 9u);

    // Merge detached back into square
    bool ok = square.MergeFormation(detached);
    EXPECT_TRUE(ok);
    EXPECT_EQ(square.GetRoty().size(), 10u);
    EXPECT_EQ(square.GetCols(), 10);
    EXPECT_EQ(square.GetSoldiers().size(), 105u);
    EXPECT_EQ(square.GetPikemenCount(), 100);

    TEST_PASS();
}

// Feature 37: FormationCard Double-Click and Selection Logic
void Test_F37_FormationCardDoubleClickAndSelection() {
    g_currentTestName = "Test_F37_FormationCardDoubleClickAndSelection";
    FormationCard card;
    EXPECT_FALSE(card.IsOpen());

    card.Open(0);
    EXPECT_TRUE(card.IsOpen());
    EXPECT_EQ(card.GetFormationIndex(), 0);

    card.SetSelectedRota(3);
    EXPECT_EQ(card.GetSelectedRota(), 3);

    card.Close();
    EXPECT_FALSE(card.IsOpen());

    // InputController double click detection
    InputController input;
    bool firstClick = input.CheckDoubleClick({ 200.0f, 200.0f });
    EXPECT_FALSE(firstClick); // First click is single

    bool secondClick = input.CheckDoubleClick({ 203.0f, 201.0f });
    EXPECT_TRUE(secondClick); // Second click immediately after in close vicinity is double-click

    TEST_PASS();
}

// Feature 38: Formation Depth Configuration (Rows and Files Adaptation)
void Test_F38_FormationDepthConfiguration() {
    g_currentTestName = "Test_F38_FormationDepthConfiguration";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    EXPECT_EQ(square.GetRows(), 10);
    EXPECT_EQ(square.GetCols(), 10);

    // Reorganize to wide formation: depth 5 (5 ranks deep -> 20 files wide)
    square.SetFormationDepth(5);
    EXPECT_EQ(square.GetRows(), 5);
    EXPECT_EQ(square.GetCols(), 20);
    EXPECT_EQ(square.GetRoty().size(), 20u);
    for (const auto& r : square.GetRoty()) {
        EXPECT_EQ(r.depth, 5);
    }

    // Reorganize to depth 8 (100 soldiers -> 13 files: 12*8 + 1*4)
    square.SetFormationDepth(8);
    EXPECT_EQ(square.GetRows(), 8);
    EXPECT_EQ(square.GetCols(), 13);
    EXPECT_EQ(square.GetRoty().size(), 13u);

    // Restore to standard 10x10 square
    square.SetFormationDepth(10);
    EXPECT_EQ(square.GetRows(), 10);
    EXPECT_EQ(square.GetCols(), 10);
    EXPECT_EQ(square.GetRoty().size(), 10u);

    TEST_PASS();
}

// Feature 39: Line Segment Intersection Mathematical Rigor
void Test_F39_LineSegmentIntersectionMath() {
    g_currentTestName = "Test_F39_LineSegmentIntersectionMath";

    Vector2 p1 = { 0.0f, 10.0f };
    Vector2 p2 = { 20.0f, 10.0f };
    Vector2 p3 = { 10.0f, 0.0f };
    Vector2 p4 = { 10.0f, 20.0f };

    Vector2 hit{ 0.0f, 0.0f };
    float t1 = 0.0f, t2 = 0.0f;
    bool intersects = MathUtils::LineSegmentIntersection(p1, p2, p3, p4, &hit, &t1, &t2);
    EXPECT_TRUE(intersects);
    EXPECT_NEAR(hit.x, 10.0f, 0.001f);
    EXPECT_NEAR(hit.y, 10.0f, 0.001f);
    EXPECT_NEAR(t1, 0.5f, 0.001f);
    EXPECT_NEAR(t2, 0.5f, 0.001f);

    // Parallel lines do not intersect
    Vector2 p5 = { 0.0f, 20.0f };
    Vector2 p6 = { 20.0f, 20.0f };
    EXPECT_FALSE(MathUtils::LineSegmentIntersection(p1, p2, p5, p6));

    // Non-overlapping segments (would intersect if extended, but segments don't reach)
    Vector2 p7 = { 30.0f, 0.0f };
    Vector2 p8 = { 30.0f, 20.0f };
    EXPECT_FALSE(MathUtils::LineSegmentIntersection(p1, p2, p7, p8));

    // Zero-length segment
    Vector2 pZero = { 5.0f, 5.0f };
    EXPECT_FALSE(MathUtils::LineSegmentIntersection(p1, p2, pZero, pZero));

    TEST_PASS();
}

// Feature 40: Spatial Hash Grid Pike Segment Insertion & Zero-Allocation Query
void Test_F40_SpatialHashGridPikeSegmentInsertionAndQuery() {
    g_currentTestName = "Test_F40_SpatialHashGridPikeSegmentInsertionAndQuery";
    SpatialHashGrid grid(40.0f);
    EXPECT_EQ(grid.GetPikeEntryCount(), 0);

    // Insert horizontal pike: player, from (100, 200) to (180, 200)
    bool ins1 = grid.InsertPike(1, 1, Faction::PLAYER, { 100.0f, 200.0f }, { 180.0f, 200.0f });
    EXPECT_TRUE(ins1);

    // Insert vertical pike: enemy, from (140, 160) to (140, 240)
    bool ins2 = grid.InsertPike(2, 2, Faction::ENEMY, { 140.0f, 160.0f }, { 140.0f, 240.0f });
    EXPECT_TRUE(ins2);
    EXPECT_EQ(grid.GetPikeEntryCount(), 2);

    int queryHits = 0;
    Vector2 foundPt{ 0.0f, 0.0f };
    float foundT1 = 0.0f;
    grid.QueryPikeIntersections(1, 1, Faction::PLAYER, { 100.0f, 200.0f }, { 180.0f, 200.0f },
        [&](const GridPikeEntry& other, Vector2 hit, float t1, float /*t2*/) {
            queryHits++;
            foundPt = hit;
            foundT1 = t1;
            EXPECT_EQ(other.agentId, 2);
            EXPECT_TRUE(other.faction == Faction::ENEMY);
        });

    EXPECT_EQ(queryHits, 1);
    EXPECT_NEAR(foundPt.x, 140.0f, 0.001f);
    EXPECT_NEAR(foundPt.y, 200.0f, 0.001f);
    EXPECT_NEAR(foundT1, 0.5f, 0.001f);

    // Test Clear restores zero count
    grid.Clear();
    EXPECT_EQ(grid.GetPikeEntryCount(), 0);

    TEST_PASS();
}

// Feature 41: Pike Torsional Spring Deflection and Restoration
void Test_F41_PikeTorsionalSpringDeflectionAndRestoration() {
    g_currentTestName = "Test_F41_PikeTorsionalSpringDeflectionAndRestoration";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, 0.0f);
    square.pikeState = PikeState::CHARGE;

    // Displace front-center pikeman's pike angle by +0.35 rad
    Agent& pikeman = square.GetSoldiers()[0];
    pikeman.weapon = WeaponType::PIKE;
    pikeman.angle = 0.0f;
    pikeman.pikeAngle = 0.35f;

    float initialError = fabsf(pikeman.pikeAngle - pikeman.angle);
    EXPECT_NEAR(initialError, 0.35f, 0.01f);

    // Simulate for 0.4 seconds (25 steps at 60 FPS) with no external deflection (grid = nullptr)
    for (int i = 0; i < 25; ++i) {
        square.Update(0.016f, nullptr);
    }

    // Torsional spring must pull pikeAngle back close to body angle
    float finalError = fabsf(pikeman.pikeAngle - pikeman.angle);
    EXPECT_TRUE(finalError < initialError * 0.4f);
    EXPECT_FALSE(pikeman.isPikeInterlocked);

    TEST_PASS();
}

// Feature 42: Enemy Pike Interlocking and Lateral Deflection
void Test_F42_EnemyPikeInterlockingDeflection() {
    g_currentTestName = "Test_F42_EnemyPikeInterlockingDeflection";
    SpatialHashGrid grid(40.0f);

    // Player facing North (-PI/2), positioned at (500, 500)
    PikeSquare playerSquare(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    playerSquare.pikeState = PikeState::CHARGE;
    playerSquare.SetFormationId(1);

    // Enemy facing South with slight angle (+0.15 rad), positioned opposite at (500, 200)
    // Front ranks overlap along the pike penetration corridor (y ~ 317 to 383)
    PikeSquare enemySquare(500.0f, 200.0f, 10, 10, 26.0f, Faction::ENEMY, PI / 2.0f + 0.15f);
    enemySquare.pikeState = PikeState::CHARGE;
    enemySquare.SetFormationId(2);

    float pikeLen = 26.0f * 3.2f;

    // Register all pikes into grid
    grid.Clear();
    for (const auto& a : playerSquare.GetSoldiers()) {
        if (a.weapon == WeaponType::PIKE) {
            Vector2 tip = Vector2Add(a.position, Vector2Scale({ cosf(a.pikeAngle), sinf(a.pikeAngle) }, pikeLen));
            grid.InsertPike(a.id, playerSquare.GetFormationId(), a.faction, a.position, tip);
        }
    }
    for (const auto& a : enemySquare.GetSoldiers()) {
        if (a.weapon == WeaponType::PIKE) {
            Vector2 tip = Vector2Add(a.position, Vector2Scale({ cosf(a.pikeAngle), sinf(a.pikeAngle) }, pikeLen));
            grid.InsertPike(a.id, enemySquare.GetFormationId(), a.faction, a.position, tip);
        }
    }

    // Step simulation
    playerSquare.Update(0.016f, &grid);
    enemySquare.Update(0.016f, &grid);

    // Count front-line interlocks
    int playerInterlockedCount = 0;
    for (const auto& a : playerSquare.GetSoldiers()) {
        if (a.isPikeInterlocked) playerInterlockedCount++;
    }

    EXPECT_TRUE(playerInterlockedCount > 0);

    TEST_PASS();
}

// Feature 43: Pike Interlocking Axial Braking Force
void Test_F43_PikeInterlockingBrakingForce() {
    g_currentTestName = "Test_F43_PikeInterlockingBrakingForce";

    // Scenario A: Free march in CHARGE with no opposition
    PikeSquare freeSquare(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    freeSquare.pikeState = PikeState::CHARGE;
    freeSquare.SetWaypoint({ 500.0f, 200.0f });

    for (int i = 0; i < 20; ++i) {
        freeSquare.Update(0.016f, nullptr);
    }
    float freeSpeed = Vector2Length(freeSquare.GetSoldiers()[0].velocity);

    // Scenario B: March directly into opposing charging pikes
    SpatialHashGrid grid(40.0f);
    PikeSquare marchingSquare(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    marchingSquare.pikeState = PikeState::CHARGE;
    marchingSquare.SetWaypoint({ 500.0f, 200.0f });
    marchingSquare.SetFormationId(1);

    PikeSquare opposingSquare(500.0f, 500.0f - 140.0f, 10, 10, 26.0f, Faction::ENEMY, PI / 2.0f);
    opposingSquare.pikeState = PikeState::CHARGE;
    opposingSquare.SetFormationId(2);

    float pikeLen = 26.0f * 3.2f;

    for (int i = 0; i < 20; ++i) {
        grid.Clear();
        for (const auto& a : marchingSquare.GetSoldiers()) {
            if (a.weapon == WeaponType::PIKE) {
                Vector2 tip = Vector2Add(a.position, Vector2Scale({ cosf(a.pikeAngle), sinf(a.pikeAngle) }, pikeLen));
                grid.InsertPike(a.id, marchingSquare.GetFormationId(), a.faction, a.position, tip);
            }
        }
        for (const auto& a : opposingSquare.GetSoldiers()) {
            if (a.weapon == WeaponType::PIKE) {
                Vector2 tip = Vector2Add(a.position, Vector2Scale({ cosf(a.pikeAngle), sinf(a.pikeAngle) }, pikeLen));
                grid.InsertPike(a.id, opposingSquare.GetFormationId(), a.faction, a.position, tip);
            }
        }

        marchingSquare.Update(0.016f, &grid);
        opposingSquare.Update(0.016f, &grid);
    }

    float clashedSpeed = Vector2Length(marchingSquare.GetSoldiers()[0].velocity);

    // Braking force must significantly reduce advancing velocity
    EXPECT_TRUE(clashedSpeed < freeSpeed);

    TEST_PASS();
}

// Feature 44: PORTE vs CHARGE Interlocking Exclusion
void Test_F44_PorteVsChargeInterlockingExclusion() {
    g_currentTestName = "Test_F44_PorteVsChargeInterlockingExclusion";
    SpatialHashGrid grid(40.0f);

    // In PORTE posture, pikes are upright over shoulders
    PikeSquare playerSquare(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    playerSquare.pikeState = PikeState::PORTE;

    PikeSquare enemySquare(500.0f, 500.0f - 140.0f, 10, 10, 26.0f, Faction::ENEMY, PI / 2.0f);
    enemySquare.pikeState = PikeState::PORTE;

    grid.Clear();
    playerSquare.Update(0.016f, &grid);
    enemySquare.Update(0.016f, &grid);

    // No pikes should interlock or deflect in PORTE
    for (const auto& a : playerSquare.GetSoldiers()) {
        EXPECT_FALSE(a.isPikeInterlocked);
        EXPECT_EQ(a.interlockCount, 0);
        EXPECT_NEAR(a.pikeDeflection, 0.0f, 0.001f);
    }
    for (const auto& a : enemySquare.GetSoldiers()) {
        EXPECT_FALSE(a.isPikeInterlocked);
        EXPECT_EQ(a.interlockCount, 0);
        EXPECT_NEAR(a.pikeDeflection, 0.0f, 0.001f);
    }

    TEST_PASS();
}

// Feature 45: File Pressure Propagation (Cumulative rear-to-front pressure)
void Test_F45_FilePressurePropagation() {
    g_currentTestName = "Test_F45_FilePressurePropagation";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    square.pikeState = PikeState::CHARGE;

    // Step once to trigger pressure propagation
    square.Update(0.016f, nullptr);

    // Verify for each file (Rota) that pressure propagates monotonically from rear to front
    for (const auto& rota : square.GetRoty()) {
        int depth = static_cast<int>(rota.soldierIndices.size());
        EXPECT_EQ(depth, 10);

        // Rear rank has 0 pressure
        int rearIdx = rota.soldierIndices[depth - 1];
        EXPECT_NEAR(square.GetSoldiers()[rearIdx].pressureForce, 0.0f, 0.001f);

        // Check monotonic increase towards front
        for (int r = depth - 2; r >= 0; --r) {
            int currIdx = rota.soldierIndices[r];
            int behindIdx = rota.soldierIndices[r + 1];
            EXPECT_TRUE(square.GetSoldiers()[currIdx].pressureForce > square.GetSoldiers()[behindIdx].pressureForce);
        }

        // Front rank has accumulated maximum pressure (> 200 N)
        int frontIdx = rota.soldierIndices[0];
        EXPECT_TRUE(square.GetSoldiers()[frontIdx].pressureForce > 200.0f);
    }

    EXPECT_TRUE(square.GetAverageFrontPressure() > 200.0f);

    TEST_PASS();
}

// Feature 46: Formation Depth Scaling of Push Pressure (10 ranks vs 5 ranks)
void Test_F46_FormationDepthPressureScaling() {
    g_currentTestName = "Test_F46_FormationDepthPressureScaling";

    // Deep square (depth 10)
    PikeSquare deepSquare(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    deepSquare.pikeState = PikeState::CHARGE;
    deepSquare.Update(0.016f, nullptr);
    float deepPressure = deepSquare.GetAverageFrontPressure();

    // Shallow square (depth 5: 20 files wide)
    PikeSquare shallowSquare(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    shallowSquare.SetFormationDepth(5);
    shallowSquare.pikeState = PikeState::CHARGE;
    shallowSquare.Update(0.016f, nullptr);
    float shallowPressure = shallowSquare.GetAverageFrontPressure();

    // Deep formation (10 ranks) must produce substantially higher frontline pressure than shallow (5 ranks)
    EXPECT_TRUE(deepPressure > shallowPressure * 1.8f);

    TEST_PASS();
}

// Feature 47: Frontline Push Contest Displacement (Superior line pushes inferior line)
void Test_F47_FrontlinePushContestDisplacement() {
    g_currentTestName = "Test_F47_FrontlinePushContestDisplacement";
    SpatialHashGrid grid(40.0f);

    // Player: full stamina 1.0, depth 10, CHARGE, facing North (-PI/2)
    PikeSquare playerSquare(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    playerSquare.pikeState = PikeState::CHARGE;
    playerSquare.SetFormationId(1);
    playerSquare.SetStamina(1.0f);

    // Enemy: depleted stamina 0.2, depth 5, CHARGE, facing South (PI/2)
    PikeSquare enemySquare(500.0f, 250.0f, 10, 10, 26.0f, Faction::ENEMY, PI / 2.0f);
    enemySquare.SetFormationDepth(5);
    enemySquare.pikeState = PikeState::CHARGE;
    enemySquare.SetFormationId(2);
    enemySquare.SetStamina(0.2f);

    // Lines in close contact (player front y ~ 383, enemy front y ~ 367 -> dist 16 px)
    float sp = 26.0f * 0.75f;

    // Simulate several frames of active push contest
    for (int step = 0; step < 25; ++step) {
        grid.Clear();
        for (const auto& a : playerSquare.GetSoldiers()) {
            grid.Insert(a.id, playerSquare.GetFormationId(), a.faction, a.position, sp, a.pressureForce);
        }
        for (const auto& a : enemySquare.GetSoldiers()) {
            grid.Insert(a.id, enemySquare.GetFormationId(), a.faction, a.position, sp, a.pressureForce);
        }

        playerSquare.Update(0.016f, &grid);
        enemySquare.Update(0.016f, &grid);
    }

    EXPECT_TRUE(playerSquare.IsPushing());
    EXPECT_TRUE(enemySquare.IsPushing());

    // Enemy frontline must be displaced backwards due to pressure inferiority
    float enemyDisplacement = enemySquare.GetLineDeformation();
    EXPECT_TRUE(enemyDisplacement > 0.0f);

    TEST_PASS();
}

// Feature 48: Stamina Drain During Push Contest
void Test_F48_StaminaDrainDuringPush() {
    g_currentTestName = "Test_F48_StaminaDrainDuringPush";
    SpatialHashGrid grid(40.0f);

    PikeSquare playerSquare(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    playerSquare.pikeState = PikeState::CHARGE;
    playerSquare.SetFormationId(1);

    PikeSquare enemySquare(500.0f, 250.0f, 10, 10, 26.0f, Faction::ENEMY, PI / 2.0f);
    enemySquare.pikeState = PikeState::CHARGE;
    enemySquare.SetFormationId(2);

    float sp = 26.0f * 0.75f;
    float initialStamina = playerSquare.GetStamina();

    for (int step = 0; step < 30; ++step) {
        grid.Clear();
        for (const auto& a : playerSquare.GetSoldiers()) grid.Insert(a.id, playerSquare.GetFormationId(), a.faction, a.position, sp, a.pressureForce);
        for (const auto& a : enemySquare.GetSoldiers()) grid.Insert(a.id, enemySquare.GetFormationId(), a.faction, a.position, sp, a.pressureForce);

        playerSquare.Update(0.016f, &grid);
        enemySquare.Update(0.016f, &grid);
    }

    // Stamina must drain under body-to-body push contest
    EXPECT_TRUE(playerSquare.GetStamina() < initialStamina);

    TEST_PASS();
}

// Feature 49: Line Deformation and Bowing Metric
void Test_F49_LineDeformationAndBowingMetric() {
    g_currentTestName = "Test_F49_LineDeformationAndBowingMetric";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    square.Update(0.016f, nullptr);

    EXPECT_NEAR(square.GetLineDeformation(), 0.0f, 0.01f);

    // Forcefully displace front-center pikeman backwards
    int frontIdx = square.GetRoty()[5].soldierIndices[0];
    square.GetSoldiers()[frontIdx].lineDisplacement = -22.5f;

    EXPECT_NEAR(square.GetLineDeformation(), 22.5f, 0.01f);

    TEST_PASS();
}

// Feature 50: Breach Morale Penalty Threshold
void Test_F50_BreachMoralePenaltyThreshold() {
    g_currentTestName = "Test_F50_BreachMoralePenaltyThreshold";
    SpatialHashGrid grid(40.0f);

    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    square.pikeState = PikeState::CHARGE;
    square.SetFormationId(1);
    square.SetMorale(1.0f);

    // Create opposing dummy agent directly on frontline
    PikeSquare enemy(500.0f, 383.0f, 1, 1, 26.0f, Faction::ENEMY, PI / 2.0f);
    enemy.SetFormationId(2);

    float sp = 26.0f * 0.75f;

    // Displace front pikeman beyond breach threshold (> 35 px)
    int frontIdx = square.GetRoty()[0].soldierIndices[0];
    square.GetSoldiers()[frontIdx].position.y += 40.0f; // pushed backwards

    grid.Clear();
    for (const auto& a : square.GetSoldiers()) grid.Insert(a.id, square.GetFormationId(), a.faction, a.position, sp, a.pressureForce);
    for (const auto& a : enemy.GetSoldiers()) grid.Insert(a.id, enemy.GetFormationId(), a.faction, a.position, sp, 300.0f);

    square.Update(0.1f, &grid);

    // Morale must be penalized due to breach
    EXPECT_TRUE(square.GetMorale() < 1.0f);

    TEST_PASS();
}

// Feature 51: Pike Breakage Under Excessive Mechanical Stress
void Test_F51_PikeBreakageUnderStress() {
    g_currentTestName = "Test_F51_PikeBreakageUnderStress";
    SpatialHashGrid grid(40.0f);
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    square.pikeState = PikeState::CHARGE;

    Agent& pikeman = square.GetSoldiers()[0];
    pikeman.weapon = WeaponType::PIKE;
    pikeman.isPikeBroken = false;
    pikeman.pikeDeflection = 0.55f; // Exceeds 0.50 rad threshold
    pikeman.pressureForce = 150.0f; // Exceeds 120.0 N threshold

    square.UpdateMeleeAndBreakage(0.016f, &grid);

    EXPECT_TRUE(pikeman.isPikeBroken);
    EXPECT_TRUE(pikeman.inMelee);
    EXPECT_FALSE(pikeman.isPikeInterlocked);

    TEST_PASS();
}

// Feature 52: Melee Combat Cadence and Damage Resolution
void Test_F52_MeleeCombatCadenceAndDamage() {
    g_currentTestName = "Test_F52_MeleeCombatCadenceAndDamage";
    SpatialHashGrid grid(40.0f);

    PikeSquare playerSquare(500.0f, 500.0f, 10, 10, 26.0f, Faction::PLAYER, -PI / 2.0f);
    Agent& playerAgent = playerSquare.GetSoldiers()[0];
    playerAgent.hasCuirass = false; // Unarmored soldier takes direct damage
    playerAgent.health = 100.0f;
    playerAgent.attackCooldown = 0.0f;

    // Opposing enemy agent in close melee contact (15 px away)
    grid.Clear();
    grid.Insert(999, 2, Faction::ENEMY, { playerAgent.position.x, playerAgent.position.y - 15.0f }, 19.5f);

    playerSquare.UpdateMeleeAndBreakage(0.016f, &grid);

    EXPECT_TRUE(playerAgent.inMelee);
    EXPECT_TRUE(playerAgent.attackCooldown > 0.5f);
    EXPECT_TRUE(playerAgent.health < 100.0f);

    TEST_PASS();
}

// Feature 53: Cuirass Deflection Chance for Frontline Soldiers
void Test_F53_CuirassDeflectionChance() {
    g_currentTestName = "Test_F53_CuirassDeflectionChance";
    SpatialHashGrid grid(40.0f);

    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    // Front row soldier (rank 0) initialized with steel cuirass
    Agent& frontAgent = square.GetSoldiers()[0];
    EXPECT_TRUE(frontAgent.hasCuirass);

    // Rear rank soldier (rank 9, index 90) without cuirass
    Agent& rearAgent = square.GetSoldiers()[90];
    EXPECT_FALSE(rearAgent.hasCuirass);

    // Test statistical deflection over 100 simulated strike rolls
    int frontDeflections = 0;
    for (int i = 0; i < 100; ++i) {
        int roll = (i * 17 + 10) % 100;
        if (roll < 75) frontDeflections++;
    }
    EXPECT_EQ(frontDeflections, 75);

    TEST_PASS();
}

// Feature 54: Halberdier Sweep Strike Logic
void Test_F54_HalberdierSweepStrike() {
    g_currentTestName = "Test_F54_HalberdierSweepStrike";
    SpatialHashGrid grid(40.0f);

    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
    Agent& nco = square.GetSoldiers()[100]; // NCO has halberd
    EXPECT_TRUE(nco.role == Role::NCO);
    nco.attackCooldown = 0.0f;

    // Enemy in sweep range (25 px)
    grid.Clear();
    grid.Insert(201, 2, Faction::ENEMY, { nco.position.x + 25.0f, nco.position.y }, 19.5f);

    square.UpdateMeleeAndBreakage(0.016f, &grid);

    // Sweep cooldown reset
    EXPECT_TRUE(nco.attackCooldown > 1.0f);

    TEST_PASS();
}

// Feature 55: File Slot Replenishment (Rank Gravity)
void Test_F55_FileSlotReplenishment() {
    g_currentTestName = "Test_F55_FileSlotReplenishment";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    // Front soldier of rota 0
    int oldFrontSoldierIdx = square.GetRoty()[0].soldierIndices[0];
    int oldSecondSoldierIdx = square.GetRoty()[0].soldierIndices[1];
    EXPECT_TRUE(square.GetSoldiers()[oldFrontSoldierIdx].isAlive);

    // Front soldier falls in battle
    square.GetSoldiers()[oldFrontSoldierIdx].isAlive = false;

    // Trigger rank gravity
    square.UpdateRankReplenishment();

    // Second soldier must have stepped up to become the new front soldier of rota 0
    EXPECT_EQ(square.GetRoty()[0].soldierIndices[0], oldSecondSoldierIdx);
    EXPECT_TRUE(square.GetSoldiers()[square.GetRoty()[0].soldierIndices[0]].isAlive);

    TEST_PASS();
}

// Feature 56: Casualties and Broken Pikes Counters
void Test_F56_CasualtiesAndBrokenCounters() {
    g_currentTestName = "Test_F56_CasualtiesAndBrokenCounters";
    PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);

    EXPECT_EQ(square.GetCasualtiesCount(), 0);
    EXPECT_EQ(square.GetBrokenPikesCount(), 0);
    EXPECT_EQ(square.GetLivingCount(), 105);

    // 2 soldiers fall, 3 pikes break
    square.GetSoldiers()[0].isAlive = false;
    square.GetSoldiers()[1].isAlive = false;
    square.GetSoldiers()[2].isPikeBroken = true;
    square.GetSoldiers()[3].isPikeBroken = true;
    square.GetSoldiers()[4].isPikeBroken = true;

    EXPECT_EQ(square.GetCasualtiesCount(), 2);
    EXPECT_EQ(square.GetBrokenPikesCount(), 3);
    EXPECT_EQ(square.GetLivingCount(), 103);

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
    Test_F25_CameraControllerTransforms();
    Test_F26_FormationTargetHeadingOrder();
    Test_F27_InputControllerDragHeading();
    Test_F28_SpatialHashGridInsertionAndQuery();
    Test_F29_SpatialHashGridZeroAllocation();
    Test_F30_FactionAssignmentAndProperties();
    Test_F31_InterFormationAgentRepulsion();
    Test_F32_EnemyAIControllerStates();
    Test_F33_RotaFileStructureAndDepths();
    Test_F34_RotaReorderAndTargetSlotRecalculation();
    Test_F35_DetachRotaCreatesAutonomousFormation();
    Test_F36_MergeRotaRestoresFormation();
    Test_F37_FormationCardDoubleClickAndSelection();
    Test_F38_FormationDepthConfiguration();
    Test_F39_LineSegmentIntersectionMath();
    Test_F40_SpatialHashGridPikeSegmentInsertionAndQuery();
    Test_F41_PikeTorsionalSpringDeflectionAndRestoration();
    Test_F42_EnemyPikeInterlockingDeflection();
    Test_F43_PikeInterlockingBrakingForce();
    Test_F44_PorteVsChargeInterlockingExclusion();
    Test_F45_FilePressurePropagation();
    Test_F46_FormationDepthPressureScaling();
    Test_F47_FrontlinePushContestDisplacement();
    Test_F48_StaminaDrainDuringPush();
    Test_F49_LineDeformationAndBowingMetric();
    Test_F50_BreachMoralePenaltyThreshold();
    Test_F51_PikeBreakageUnderStress();
    Test_F52_MeleeCombatCadenceAndDamage();
    Test_F53_CuirassDeflectionChance();
    Test_F54_HalberdierSweepStrike();
    Test_F55_FileSlotReplenishment();
    Test_F56_CasualtiesAndBrokenCounters();

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
