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
    m_pikeEntryCount = 0;
    std::memset(m_pikeHead, -1, sizeof(m_pikeHead));
}

bool SpatialHashGrid::Insert(AgentId agentId, FormationId formationId, Faction faction, Vector2 position, float radius, float pressureForce) {
    if (m_entryCount >= MAX_ENTRIES) {
        return false;
    }

    int cx = static_cast<int>(std::floor(position.x * m_invCellSize));
    int cy = static_cast<int>(std::floor(position.y * m_invCellSize));
    int bucket = HashCoords(cx, cy);

    int id = m_entryCount++;
    m_entries[id] = GridAgentEntry{ agentId, formationId, faction, position, radius, pressureForce, cx, cy };
    m_next[id] = m_head[bucket];
    m_head[bucket] = id;

    return true;
}

bool SpatialHashGrid::InsertPike(AgentId agentId, FormationId formationId, Faction faction, Vector2 start, Vector2 end) {
    if (m_pikeEntryCount >= MAX_PIKE_ENTRIES) {
        return false;
    }

    Vector2 d = { end.x - start.x, end.y - start.y };
    float len = Vector2Length(d);
    if (len <= 0.001f) return false;

    Vector2 mid = { start.x + d.x * 0.5f, start.y + d.y * 0.5f };
    int cx = static_cast<int>(std::floor(mid.x * m_invCellSize));
    int cy = static_cast<int>(std::floor(mid.y * m_invCellSize));
    int bucket = HashCoords(cx, cy);

    int idx = m_pikeEntryCount++;
    m_pikeEntries[idx] = GridPikeEntry{
        agentId,
        formationId,
        faction,
        start,
        end,
        mid,
        len * 0.5f,
        cx,
        cy
    };
    m_pikeNext[idx] = m_pikeHead[bucket];
    m_pikeHead[bucket] = idx;

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
