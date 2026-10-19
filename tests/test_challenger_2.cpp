#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <chrono>

#include "raylib.h"

// Prevent windows.h conflicts with raylib
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#include <psapi.h>
#undef near
#undef far
#undef PlaySound
#undef DrawText
#undef CloseWindow
#undef ShowCursor
#undef Rectangle

#include "core/Types.h"
#include "core/MathUtils.h"
#include "simulation/Agent.h"
#include "formation/PikeSquare.h"
#include "rendering/IRenderer.h"
#include "rendering/EngravingRenderer.h"
#include "rendering/VectorRenderer.h"
#include "ui/UIManager.h"

// ============================================================================
// GLOBAL ALLOCATION TRACKING (Adversarial Memory Invariant Verifier)
// ============================================================================
static size_t g_heapAllocCount = 0;
static size_t g_heapAllocBytes = 0;
static bool g_trackAllocations = false;

void* operator new(size_t size) {
    if (g_trackAllocations) {
        g_heapAllocCount++;
        g_heapAllocBytes += size;
        std::cerr << "  [ALLOC DETECTED] operator new called for " << size << " bytes!\n";
    }
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void operator delete(void* ptr) noexcept {
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

void* operator new[](size_t size) {
    if (g_trackAllocations) {
        g_heapAllocCount++;
        g_heapAllocBytes += size;
        std::cerr << "  [ALLOC DETECTED] operator new[] called for " << size << " bytes!\n";
    }
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void operator delete[](void* ptr) noexcept {
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    std::free(ptr);
}

// ============================================================================
// HELPER STRUCTURE FOR EXACT AGENT SNAPSHOT
// ============================================================================
struct AgentSnapshot {
    int id;
    Role role;
    Vector2 position;
    Vector2 velocity;
    Vector2 targetSlot;
    float angle;
    float pikeAngle;
    float swingTimer;
    float mass;
    float maxForce;

    bool operator==(const AgentSnapshot& o) const {
        return id == o.id &&
               role == o.role &&
               position.x == o.position.x && position.y == o.position.y &&
               velocity.x == o.velocity.x && velocity.y == o.velocity.y &&
               targetSlot.x == o.targetSlot.x && targetSlot.y == o.targetSlot.y &&
               angle == o.angle &&
               pikeAngle == o.pikeAngle &&
               swingTimer == o.swingTimer &&
               mass == o.mass &&
               maxForce == o.maxForce;
    }
};

static std::vector<AgentSnapshot> CaptureSnapshots(const PikeSquare& square) {
    std::vector<AgentSnapshot> snaps;
    snaps.reserve(square.GetSoldiers().size());
    for (const auto& a : square.GetSoldiers()) {
        snaps.push_back({
            a.id, a.role, a.position, a.velocity, a.targetSlot,
            a.angle, a.pikeAngle, a.swingTimer, a.mass, a.maxForce
        });
    }
    return snaps;
}

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  PIKESIM CHALLENGER 2: ADVERSARIAL VERIFICATION HARNESS\n";
    std::cout << "=========================================================\n\n";

    // Initialize hidden raylib window for rendering validation
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1200, 800, "Headless Challenger Window");

    bool allTestsPassed = true;

    // ------------------------------------------------------------------------
    // CHALLENGE 1: IRenderer::Render(const PikeSquare&) Read-Only Invariant
    // ------------------------------------------------------------------------
    std::cout << "[CHALLENGE 1] Verifying IRenderer::Render(const PikeSquare&) strictly read-only...\n";
    {
        PikeSquare square(500.0f, 400.0f, 10, 10, 26.0f);
        square.SetWaypoint({ 650.0f, 350.0f });

        // Step simulation 10 ticks to create non-trivial dynamic state
        for (int i = 0; i < 10; ++i) square.Update(0.016f);

        // Capture baseline state
        Vector2 basePos = square.GetPosition();
        float baseAngle = square.GetAngle();
        PikeState baseState = square.GetState();
        bool baseHasWp = square.HasWaypoint();
        Vector2 baseWp = square.GetWaypoint();
        auto baseAgents = CaptureSnapshots(square);

        EngravingRenderer engraving;
        VectorRenderer vector;

        // Render through polymorphic IRenderer interface
        IRenderer* r1 = &engraving;
        IRenderer* r2 = &vector;

        BeginDrawing();
        r1->Render(square);
        r2->Render(square);
        r1->DrawScene(square);
        r2->DrawScene(square);
        EndDrawing();

        // Verify that NO state was mutated
        bool statePreserved = true;
        if (square.GetPosition().x != basePos.x || square.GetPosition().y != basePos.y) statePreserved = false;
        if (square.GetAngle() != baseAngle) statePreserved = false;
        if (square.GetState() != baseState) statePreserved = false;
        if (square.HasWaypoint() != baseHasWp) statePreserved = false;
        if (square.GetWaypoint().x != baseWp.x || square.GetWaypoint().y != baseWp.y) statePreserved = false;

        auto afterAgents = CaptureSnapshots(square);
        if (baseAgents.size() != afterAgents.size()) statePreserved = false;
        for (size_t i = 0; i < baseAgents.size(); ++i) {
            if (!(baseAgents[i] == afterAgents[i])) {
                statePreserved = false;
                std::cerr << "  Agent " << i << " mutated by renderer!\n";
                break;
            }
        }

        if (statePreserved) {
            std::cout << "  [PASS] IRenderer::Render(const PikeSquare&) strictly read-only. 0 fields mutated.\n";
        } else {
            std::cerr << "  [FAIL] IRenderer::Render mutated simulation state!\n";
            allTestsPassed = false;
        }
    }

    // ------------------------------------------------------------------------
    // CHALLENGE 2: VisualMode Alternation & State Isolation
    // ------------------------------------------------------------------------
    std::cout << "\n[CHALLENGE 2] Verifying alternating VisualMode has ZERO state leakage...\n";
    {
        // Run two identical squares for 300 frames:
        // Square A: alternates between ENGRAVING and VECTOR every single frame
        // Square B: pure headless simulation (no renderers called)
        PikeSquare squareA(600.0f, 400.0f, 10, 10, 26.0f);
        PikeSquare squareB(600.0f, 400.0f, 10, 10, 26.0f);

        EngravingRenderer engraving;
        VectorRenderer vector;

        squareA.SetWaypoint({ 800.0f, 250.0f });
        squareB.SetWaypoint({ 800.0f, 250.0f });

        VisualMode mode = VisualMode::ENGRAVING;
        bool identicalAcrossAllFrames = true;

        for (int frame = 0; frame < 300; ++frame) {
            // Mode toggle
            mode = (mode == VisualMode::ENGRAVING) ? VisualMode::VECTOR : VisualMode::ENGRAVING;

            squareA.Update(0.016f);
            squareB.Update(0.016f);

            BeginDrawing();
            if (mode == VisualMode::ENGRAVING) {
                engraving.DrawScene(squareA);
            } else {
                vector.DrawScene(squareA);
            }
            EndDrawing();

            // Compare states
            if (squareA.GetPosition().x != squareB.GetPosition().x ||
                squareA.GetPosition().y != squareB.GetPosition().y ||
                squareA.GetAngle() != squareB.GetAngle() ||
                squareA.HasWaypoint() != squareB.HasWaypoint()) {
                identicalAcrossAllFrames = false;
                std::cerr << "  Deviation detected at frame " << frame << "!\n";
                break;
            }

            for (size_t i = 0; i < squareA.GetSoldiers().size(); ++i) {
                const auto& a = squareA.GetSoldiers()[i];
                const auto& b = squareB.GetSoldiers()[i];
                if (a.position.x != b.position.x || a.position.y != b.position.y ||
                    a.velocity.x != b.velocity.x || a.velocity.y != b.velocity.y) {
                    identicalAcrossAllFrames = false;
                    std::cerr << "  Agent " << i << " state mismatch at frame " << frame << "!\n";
                    break;
                }
            }
            if (!identicalAcrossAllFrames) break;
        }

        if (identicalAcrossAllFrames) {
            std::cout << "  [PASS] 300 frames of alternating modes match pure simulation bit-for-bit. Zero leakage.\n";
        } else {
            std::cerr << "  [FAIL] State leakage detected during visual mode alternation!\n";
            allTestsPassed = false;
        }
    }

    // ------------------------------------------------------------------------
    // CHALLENGE 3: UIManager HUD Calculations Purity
    // ------------------------------------------------------------------------
    std::cout << "\n[CHALLENGE 3] Verifying UIManager HUD calculations do not alter agent state...\n";
    {
        PikeSquare square(500.0f, 500.0f, 10, 10, 26.0f);
        // Perturb formation to generate non-trivial tempo and cohesion
        square.GetSoldiers()[100].position.x += 25.0f;
        square.GetSoldiers()[0].velocity = { 15.0f, -20.0f };
        square.GetSoldiers()[50].position.y += 18.0f;

        auto beforeAgents = CaptureSnapshots(square);

        float tempo1 = square.GetNcoTempoFactor();
        float cohesion1 = square.GetCohesionIndex();

        // Call calculations 1000 times
        for (int i = 0; i < 1000; ++i) {
            float t = square.GetNcoTempoFactor();
            float c = square.GetCohesionIndex();
            if (t != tempo1 || c != cohesion1) {
                std::cerr << "  Non-deterministic calculation at iteration " << i << "!\n";
                allTestsPassed = false;
                break;
            }
        }

        // Call UIManager::DrawUI 100 times
        BeginDrawing();
        for (int i = 0; i < 100; ++i) {
            UIManager::DrawUI(square, VisualMode::ENGRAVING);
            UIManager::DrawUI(square, VisualMode::VECTOR);
        }
        EndDrawing();

        auto afterAgents = CaptureSnapshots(square);
        bool agentsUnchanged = true;
        for (size_t i = 0; i < beforeAgents.size(); ++i) {
            if (!(beforeAgents[i] == afterAgents[i])) {
                agentsUnchanged = false;
                std::cerr << "  Agent " << i << " altered by HUD calculations or DrawUI!\n";
                break;
            }
        }

        if (agentsUnchanged) {
            std::cout << "  [PASS] GetNcoTempoFactor and GetCohesionIndex are pure, idempotent, and cause 0 mutations.\n";
        } else {
            std::cerr << "  [FAIL] HUD calculations modified agent positions or velocities!\n";
            allTestsPassed = false;
        }
    }

    // ------------------------------------------------------------------------
    // CHALLENGE 4: 1000+ Ticks Runtime Memory Invariants & Performance
    // ------------------------------------------------------------------------
    std::cout << "\n[CHALLENGE 4] Verifying 1000+ ticks zero-allocation invariant & performance stability...\n";
    {
        PikeSquare square(600.0f, 400.0f, 10, 10, 26.0f);

        // Get baseline process memory
        PROCESS_MEMORY_COUNTERS_EX pmcStart;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmcStart, sizeof(pmcStart));
        SIZE_T baseWorkingSet = pmcStart.WorkingSetSize;

        const int TOTAL_TICKS = 2000;
        std::vector<double> batchTimes(TOTAL_TICKS / 100, 0.0);

        g_heapAllocCount = 0;
        g_heapAllocBytes = 0;

        auto totalStart = std::chrono::high_resolution_clock::now();
        auto batchStart = totalStart;

        for (int tick = 1; tick <= TOTAL_TICKS; ++tick) {
            // Enable allocation tracking exclusively during simulation updates and maneuvers
            g_trackAllocations = true;

            if (tick % 250 == 50) {
                square.InPlaceTurn90(true);
            } else if (tick % 250 == 100) {
                square.Wheel(0.02f, true, 0.016f);
            } else if (tick % 250 == 150) {
                square.SetWaypoint({ 700.0f, 300.0f });
            } else if (tick % 250 == 200) {
                square.ClearWaypoint();
            } else {
                square.Move({ 0.0f, -1.0f }, 0.016f, 80.0f);
            }

            square.Update(0.016f);

            g_trackAllocations = false;

            if (tick % 100 == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                double ms = std::chrono::duration<double, std::milli>(now - batchStart).count();
                batchTimes[(tick / 100) - 1] = ms;
                batchStart = now;
            }
        }

        auto totalEnd = std::chrono::high_resolution_clock::now();
        double totalMs = std::chrono::duration<double, std::milli>(totalEnd - totalStart).count();

        // Get final process memory
        PROCESS_MEMORY_COUNTERS_EX pmcEnd;
        GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmcEnd, sizeof(pmcEnd));
        SIZE_T endWorkingSet = pmcEnd.WorkingSetSize;
        LONG_PTR memDiff = static_cast<LONG_PTR>(endWorkingSet) - static_cast<LONG_PTR>(baseWorkingSet);

        std::cout << "  Total ticks simulated: " << TOTAL_TICKS << "\n";
        std::cout << "  Heap allocations during " << TOTAL_TICKS << " ticks: " << g_heapAllocCount << " (bytes: " << g_heapAllocBytes << ")\n";
        std::cout << "  Process Working Set: Start = " << (baseWorkingSet / 1024) << " KB, End = " << (endWorkingSet / 1024) 
                  << " KB (Delta = " << (memDiff / 1024) << " KB)\n";
        std::cout << "  Total simulation time: " << totalMs << " ms (" << (totalMs / TOTAL_TICKS) << " ms/tick)\n";

        // Performance degradation check: compare first 5 batches vs last 5 batches
        double first5Sum = 0.0;
        double last5Sum = 0.0;
        for (int i = 0; i < 5; ++i) first5Sum += batchTimes[i];
        for (int i = 15; i < 20; ++i) last5Sum += batchTimes[i];
        double first5Avg = first5Sum / 5.0;
        double last5Avg = last5Sum / 5.0;

        std::cout << "  Batch average time (100 ticks): First 500 ticks = " << first5Avg << " ms, Last 500 ticks = " << last5Avg << " ms\n";

        bool allocInvariantPass = (g_heapAllocCount == 0);
        bool perfDegradationPass = (last5Avg < first5Avg * 1.5); // Must not slow down by > 50%

        if (allocInvariantPass) {
            std::cout << "  [PASS] Zero dynamic allocations (0 calls to operator new/new[]) during 2000 ticks!\n";
        } else {
            std::cerr << "  [FAIL] Dynamic allocations detected in simulation loop!\n";
            allTestsPassed = false;
        }

        if (perfDegradationPass) {
            std::cout << "  [PASS] Performance is stable with no degradation over 2000 ticks.\n";
        } else {
            std::cerr << "  [FAIL] Performance degradation detected!\n";
            allTestsPassed = false;
        }
    }

    ::CloseWindow();

    std::cout << "\n=========================================================\n";
    if (allTestsPassed) {
        std::cout << ">>> CHALLENGER 2 VERIFICATION: ALL CHALLENGES PASSED! <<<\n";
        return 0;
    } else {
        std::cerr << ">>> CHALLENGER 2 VERIFICATION: CHALLENGES FAILED! <<<\n";
        return 1;
    }
}
