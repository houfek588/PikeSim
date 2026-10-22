#include "core/SpatialHashGrid.h"

SpatialHashGrid::SpatialHashGrid(float cellSize)
    : m_cellSize(cellSize > 1.0f ? cellSize : 40.0f)
    , m_invCellSize(1.0f / m_cellSize)
    , m_entryCount(0)
{
    Clear();
}

void SpatialHashGrid::Clear() {
    m_entryCount = 0;
    std::memset(m_head, -1, sizeof(m_head));
}

bool SpatialHashGrid::Insert(AgentId agentId, FormationId formationId, Faction faction, Vector2 position, float radius) {
    if (m_entryCount >= MAX_ENTRIES) {
        return false;
    }

    int cx = static_cast<int>(std::floor(position.x * m_invCellSize));
    int cy = static_cast<int>(std::floor(position.y * m_invCellSize));
    int bucket = HashCoords(cx, cy);

    int id = m_entryCount++;
    m_entries[id] = GridAgentEntry{ agentId, formationId, faction, position, radius, cx, cy };
    m_next[id] = m_head[bucket];
    m_head[bucket] = id;

    return true;
}

int SpatialHashGrid::QueryRadiusBuffer(Vector2 pos, float radius, GridAgentEntry* outBuffer, int maxBuffer) const {
    if (!outBuffer || maxBuffer <= 0) return 0;

    int count = 0;
    QueryRadius(pos, radius, [&](const GridAgentEntry& entry, float /*dist*/) {
        if (count < maxBuffer) {
            outBuffer[count++] = entry;
        }
    });

    return count;
}
