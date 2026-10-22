#pragma once

#include "raylib.h"
#include "core/Types.h"
#include <cmath>
#include <cstdint>
#include <cstring>

// --- STRUKTURA ZÁZNAMU AGENTA V PROSTOROVÉ MŘÍŽCE ---
struct GridAgentEntry {
    AgentId agentId{ 0 };
    FormationId formationId{ 0 };
    Faction faction{ Faction::PLAYER };
    Vector2 position{ 0.0f, 0.0f };
    float radius{ 0.0f };
    int cx{ 0 };
    int cy{ 0 };
};

// --- PROSTOROVÁ HASH MŘÍŽKA S O(N) ŠKÁLOVÁNÍM A NULOVÝMI DYNAMICKÝMI ALOKACEMI ---
class SpatialHashGrid {
public:
    static constexpr int BUCKET_COUNT = 1024;
    static constexpr int MAX_ENTRIES = 1024;

    explicit SpatialHashGrid(float cellSize = 40.0f);

    // Vyčištění mřížky na začátku simulačního snímku (O(B), trvá méně než 1 mikrosekundu)
    void Clear();

    // Vložení agenta do mřížky (O(1))
    bool Insert(AgentId agentId, FormationId formationId, Faction faction, Vector2 position, float radius);

    // Šablonový visitor pro dotazy bez alokací (inlinovaný, zero heap overhead)
    template<typename Visitor>
    void QueryRadius(Vector2 pos, float radius, Visitor&& visitor) const {
        float radiusSq = radius * radius;
        int minCx = static_cast<int>(std::floor((pos.x - radius) * m_invCellSize));
        int maxCx = static_cast<int>(std::floor((pos.x + radius) * m_invCellSize));
        int minCy = static_cast<int>(std::floor((pos.y - radius) * m_invCellSize));
        int maxCy = static_cast<int>(std::floor((pos.y + radius) * m_invCellSize));

        for (int cy = minCy; cy <= maxCy; ++cy) {
            for (int cx = minCx; cx <= maxCx; ++cx) {
                int bucket = HashCoords(cx, cy);
                int entryIdx = m_head[bucket];
                while (entryIdx != -1) {
                    const auto& entry = m_entries[entryIdx];
                    if (entry.cx == cx && entry.cy == cy) {
                        float dx = pos.x - entry.position.x;
                        float dy = pos.y - entry.position.y;
                        float distSq = dx * dx + dy * dy;
                        if (distSq <= radiusSq && distSq > 0.0001f) {
                            visitor(entry, std::sqrt(distSq));
                        }
                    }
                    entryIdx = m_next[entryIdx];
                }
            }
        }
    }

    // Pomocná metoda pro testy (vyplnění předalokovaného pole)
    int QueryRadiusBuffer(Vector2 pos, float radius, GridAgentEntry* outBuffer, int maxBuffer) const;

    int GetEntryCount() const { return m_entryCount; }
    float GetCellSize() const { return m_cellSize; }

private:
    float m_cellSize{ 40.0f };
    float m_invCellSize{ 1.0f / 40.0f };

    int m_entryCount{ 0 };
    int m_head[BUCKET_COUNT];
    int m_next[MAX_ENTRIES];
    GridAgentEntry m_entries[MAX_ENTRIES];

    static int HashCoords(int cx, int cy) {
        uint32_t h = (static_cast<uint32_t>(cx) * 73856093u) ^ (static_cast<uint32_t>(cy) * 19349663u);
        return static_cast<int>(h & (BUCKET_COUNT - 1));
    }
};
