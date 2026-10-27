#include "formation/PikeSquare.h"
#include "core/MathUtils.h"
#include "core/SpatialHashGrid.h"
#include <cmath>
#include <iostream>

PikeSquare::PikeSquare(float x, float y, int c, int r, float space, Faction fac, float initAngle)
    : formationId{ 0 }
    , faction{ fac }
    , position{ x, y }
    , angle{ MathUtils::NormalizeAngle(initAngle) }
    , targetAngle{ MathUtils::NormalizeAngle(initAngle) }
    , targetPos{ x, y }
    , hasWaypoint{ false }
    , hasTargetHeading{ false }
    , targetHeading{ MathUtils::NormalizeAngle(initAngle) }
    , cols{ c }
    , rows{ r }
    , spacing{ space }
    , pikeState{ PikeState::PORTE }
    , m_turnBuffer(c * r)
{
    soldiers.reserve(cols * rows + 5);
    int idCounter = 0;

    // 1. INICIALIZACE PLNÉHO BLOKU PIKENÝRŮ (100 vojáků pro 10x10)
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            Agent agent;
            agent.id = idCounter++;
            agent.role = Role::PIKEMAN;
            agent.faction = faction;
            agent.mass = 1.0f;
            agent.maxForce = 180.0f;
            agent.hasCuirass = (row == 0); // Přední řada (Kozáci) má ocelové kyrysy
            agent.position = position;
            agent.velocity = { 0.0f, 0.0f };
            agent.angle = angle;
            agent.pikeAngle = angle;
            agent.swingTimer = static_cast<float>(GetRandomValue(0, 100)) / 10.0f;
            soldiers.push_back(agent);
        }
    }

    // 2. PŘIDÁNÍ PODDŮSTOJNÍKŮ (NCOs) - 4 rohy vně formace
    for (int i = 0; i < 4; ++i) {
        Agent agent;
        agent.id = idCounter++;
        agent.role = Role::NCO;
        agent.faction = faction;
        agent.mass = 1.0f;
        agent.maxForce = 180.0f;
        agent.hasCuirass = true;
        agent.position = position;
        agent.velocity = { 0.0f, 0.0f };
        agent.angle = angle;
        agent.pikeAngle = angle;
        agent.swingTimer = static_cast<float>(GetRandomValue(0, 100)) / 10.0f;
        soldiers.push_back(agent);
    }

    // 3. PŘIDÁNÍ VELITELE (Leader) - centrálně za zadní řadou
    {
        Agent agent;
        agent.id = idCounter++;
        agent.role = Role::LEADER;
        agent.faction = faction;
        agent.mass = 1.0f;
        agent.maxForce = 180.0f;
        agent.hasCuirass = true;
        agent.position = position;
        agent.velocity = { 0.0f, 0.0f };
        agent.angle = angle;
        agent.pikeAngle = angle;
        agent.swingTimer = static_cast<float>(GetRandomValue(0, 100)) / 10.0f;
        soldiers.push_back(agent);
    }

    // 4. Inicializace zástupů (Rot)
    roty.reserve(cols);
    for (int col = 0; col < cols; ++col) {
        Rota r;
        r.id = col;
        r.weaponType = WeaponType::PIKE;
        r.depth = rows;
        r.soldierIndices.reserve(rows);
        for (int row = 0; row < rows; ++row) {
            r.soldierIndices.push_back(row * cols + col);
        }
        roty.push_back(r);
    }

    // Výpočet slotů a umístění vojáků na výchozí pozice
    UpdateTargetSlots();
    for (auto& agent : soldiers) {
        agent.position = agent.targetSlot;
    }
}

Vector2 PikeSquare::GetFrontCenter() const {
    float rowOffset = (rows - 1) / 2.0f;
    float frontLocalY = -rowOffset * spacing;
    float rotAngle = angle + PI / 2.0f;
    Vector2 rotatedOffset = MathUtils::RotateVector({ 0.0f, frontLocalY }, rotAngle);
    return Vector2Add(position, rotatedOffset);
}

void PikeSquare::RotateAroundLocalPoint(float dAngle, Vector2 localPivot) {
    float rotAngleOld = angle + PI / 2.0f;
    Vector2 rotatedOffsetOld = MathUtils::RotateVector(localPivot, rotAngleOld);
    Vector2 globalPivot = Vector2Add(position, rotatedOffsetOld);

    angle += dAngle;

    float rotAngleNew = angle + PI / 2.0f;
    Vector2 rotatedOffsetNew = MathUtils::RotateVector(localPivot, rotAngleNew);
    position = Vector2Subtract(globalPivot, rotatedOffsetNew);
    angle = MathUtils::NormalizeAngle(angle);
}

void PikeSquare::UpdateTargetSlots() {
    cols = static_cast<int>(roty.size());
    float colOffset = (cols > 0 ? (cols - 1) / 2.0f : 0.0f);
    float rowOffset = (rows > 0 ? (rows - 1) / 2.0f : 0.0f);
    float rotAngle = angle + PI / 2.0f;

    // 1. Sloty pro vojáky v jednotlivých zástupech (rotách)
    for (int c = 0; c < cols; ++c) {
        const auto& rota = roty[c];
        int depth = static_cast<int>(rota.soldierIndices.size());
        for (int r = 0; r < depth; ++r) {
            int soldierIdx = rota.soldierIndices[r];
            if (soldierIdx >= 0 && soldierIdx < static_cast<int>(soldiers.size())) {
                float localX = (c - colOffset) * spacing;
                float localY = (r - rowOffset) * spacing;

                Vector2 rotatedOffset = MathUtils::RotateVector({ localX, localY }, rotAngle);
                soldiers[soldierIdx].targetSlot = Vector2Add(position, rotatedOffset);
                soldiers[soldierIdx].weapon = rota.weaponType;
            }
        }
    }

    // 2. Sloty pro NCOs a Velitele (následují za řadovými vojáky)
    int ncoStartIndex = 0;
    for (const auto& r : roty) {
        ncoStartIndex += static_cast<int>(r.soldierIndices.size());
    }

    float ncoOutFactor = 1.0f;
    float cornerOffsets[4][2] = {
        { -colOffset - ncoOutFactor, -rowOffset - ncoOutFactor }, // Přední levý (FL)
        {  colOffset + ncoOutFactor, -rowOffset - ncoOutFactor }, // Přední pravý (FR)
        { -colOffset - ncoOutFactor,  rowOffset + ncoOutFactor }, // Zadní levý (BL)
        {  colOffset + ncoOutFactor,  rowOffset + ncoOutFactor }  // Zadní pravý (BR)
    };

    int currIdx = ncoStartIndex;
    for (int i = 0; i < 4 && currIdx < static_cast<int>(soldiers.size()); ++i) {
        if (soldiers[currIdx].role == Role::NCO) {
            float localX = cornerOffsets[i][0] * spacing;
            float localY = cornerOffsets[i][1] * spacing;
            Vector2 rotatedOffset = MathUtils::RotateVector({ localX, localY }, rotAngle);
            soldiers[currIdx].targetSlot = Vector2Add(position, rotatedOffset);
            currIdx++;
        }
    }

    if (currIdx < static_cast<int>(soldiers.size()) && soldiers[currIdx].role == Role::LEADER) {
        float localX = 0.0f;
        float localY = (rowOffset + 1.4f) * spacing;
        Vector2 rotatedOffset = MathUtils::RotateVector({ localX, localY }, rotAngle);
        soldiers[currIdx].targetSlot = Vector2Add(position, rotatedOffset);
    }
}

void PikeSquare::SwapRoty(int idxA, int idxB) {
    if (idxA >= 0 && idxA < static_cast<int>(roty.size()) &&
        idxB >= 0 && idxB < static_cast<int>(roty.size()) &&
        idxA != idxB) {
        std::swap(roty[idxA], roty[idxB]);
        UpdateTargetSlots();
    }
}

void PikeSquare::SetFormationDepth(int targetDepth) {
    if (targetDepth < 2) targetDepth = 2;
    if (targetDepth > 20) targetDepth = 20;

    int nRankAndFile = 0;
    for (const auto& r : roty) {
        nRankAndFile += static_cast<int>(r.soldierIndices.size());
    }
    if (nRankAndFile == 0) return;

    int newCols = (nRankAndFile + targetDepth - 1) / targetDepth;
    if (newCols <= 0) newCols = 1;

    WeaponType domWeapon = WeaponType::PIKE;
    if (!roty.empty()) domWeapon = roty[0].weaponType;

    roty.clear();
    roty.reserve(newCols);

    int currSoldierIdx = 0;
    for (int c = 0; c < newCols; ++c) {
        Rota r;
        r.id = c;
        r.weaponType = domWeapon;
        int depthThisCol = std::min(targetDepth, nRankAndFile - currSoldierIdx);
        r.depth = depthThisCol;
        r.soldierIndices.reserve(depthThisCol);
        for (int d = 0; d < depthThisCol; ++d) {
            r.soldierIndices.push_back(currSoldierIdx++);
        }
        roty.push_back(r);
    }

    cols = newCols;
    rows = targetDepth;
    UpdateTargetSlots();
}

PikeSquare PikeSquare::DetachRota(int rotaIndex) {
    if (rotaIndex < 0 || rotaIndex >= static_cast<int>(roty.size()) || roty.size() <= 1) {
        return *this;
    }

    Rota targetRota = roty[rotaIndex];
    int depth = static_cast<int>(targetRota.soldierIndices.size());

    // Nová pozice: odsunutá na křídlo (např. o 5 sloupců do boku)
    float colOffset = (cols - 1) / 2.0f;
    float flankLocalX = (rotaIndex >= cols / 2 ? (colOffset + 3.0f) : (-colOffset - 3.0f)) * spacing;
    float rotAngle = angle + PI / 2.0f;
    Vector2 offsetWorld = MathUtils::RotateVector({ flankLocalX, 0.0f }, rotAngle);
    Vector2 newPos = Vector2Add(position, offsetWorld);

    // Vytvoření nového odloučeného oddílu o 1 rotě
    PikeSquare detached(newPos.x, newPos.y, 1, depth, spacing, faction, angle);
    detached.SetName(name + " - Rota " + std::to_string(targetRota.id + 1));
    detached.SetFormationId(formationId + 100 + targetRota.id);
    detached.roty[0].weaponType = targetRota.weaponType;

    // Přeneseme stav vojáků
    for (int r = 0; r < depth; ++r) {
        int oldIdx = targetRota.soldierIndices[r];
        if (oldIdx >= 0 && oldIdx < static_cast<int>(soldiers.size())) {
            detached.soldiers[r].position = soldiers[oldIdx].position;
            detached.soldiers[r].velocity = soldiers[oldIdx].velocity;
            detached.soldiers[r].weapon = soldiers[oldIdx].weapon;
            detached.soldiers[r].angle = soldiers[oldIdx].angle;
            detached.soldiers[r].pikeAngle = soldiers[oldIdx].pikeAngle;
        }
    }

    // Odstranění zástupu z mateřské formace
    std::vector<bool> toRemove(soldiers.size(), false);
    for (int idx : targetRota.soldierIndices) {
        if (idx >= 0 && idx < static_cast<int>(soldiers.size())) {
            toRemove[idx] = true;
        }
    }

    std::vector<Agent> newSoldiers;
    newSoldiers.reserve(soldiers.size() - depth);
    std::vector<int> indexMap(soldiers.size(), -1);

    for (size_t i = 0; i < soldiers.size(); ++i) {
        if (!toRemove[i]) {
            indexMap[i] = static_cast<int>(newSoldiers.size());
            newSoldiers.push_back(soldiers[i]);
        }
    }
    soldiers = std::move(newSoldiers);

    // Smazání roty z roty[]
    roty.erase(roty.begin() + rotaIndex);

    // Přemapování soldierIndices ve zbývajících rotách
    for (auto& r : roty) {
        for (auto& sIdx : r.soldierIndices) {
            if (sIdx >= 0 && sIdx < static_cast<int>(indexMap.size())) {
                sIdx = indexMap[sIdx];
            }
        }
    }

    cols = static_cast<int>(roty.size());
    UpdateTargetSlots();
    detached.UpdateTargetSlots();

    return detached;
}

bool PikeSquare::MergeFormation(PikeSquare& other) {
    if (other.roty.empty() || other.faction != faction) return false;

    // Vložíme vojáky před NCOs a Leader
    int baseSoldierIdx = static_cast<int>(soldiers.size()) - 5;
    if (baseSoldierIdx < 0) baseSoldierIdx = static_cast<int>(soldiers.size());

    for (const auto& otherRota : other.roty) {
        Rota newRota;
        newRota.id = static_cast<int>(roty.size());
        newRota.weaponType = otherRota.weaponType;
        newRota.depth = otherRota.depth;

        for (int oIdx : otherRota.soldierIndices) {
            if (oIdx >= 0 && oIdx < static_cast<int>(other.soldiers.size())) {
                Agent a = other.soldiers[oIdx];
                int insertPos = baseSoldierIdx++;
                soldiers.insert(soldiers.begin() + insertPos, a);
                newRota.soldierIndices.push_back(insertPos);

                for (auto& r : roty) {
                    for (auto& sIdx : r.soldierIndices) {
                        if (sIdx >= insertPos) sIdx++;
                    }
                }
            }
        }
        roty.push_back(newRota);
    }

    cols = static_cast<int>(roty.size());
    UpdateTargetSlots();
    return true;
}

int PikeSquare::GetPikemenCount() const {
    int count = 0;
    for (const auto& r : roty) {
        if (r.weaponType == WeaponType::PIKE) count += static_cast<int>(r.soldierIndices.size());
    }
    return count;
}

int PikeSquare::GetMusketeerCount() const {
    int count = 0;
    for (const auto& r : roty) {
        if (r.weaponType == WeaponType::MUSKET) count += static_cast<int>(r.soldierIndices.size());
    }
    return count;
}

int PikeSquare::GetHalberdierCount() const {
    int count = 0;
    for (const auto& r : roty) {
        if (r.weaponType == WeaponType::HALBERD) count += static_cast<int>(r.soldierIndices.size());
    }
    return count;
}

float PikeSquare::GetNcoTempoFactor() const {
    float maxDev = 0.0f;

    for (const auto& agent : soldiers) {
        if (agent.role == Role::NCO) {
            float dist = Vector2Distance(agent.position, agent.targetSlot);
            if (dist > maxDev) {
                maxDev = dist;
            }
        }
    }

    float startLimit = spacing * 0.40f;
    float stopLimit = spacing * 2.0f;

    if (maxDev <= startLimit) return 1.0f;
    if (maxDev >= stopLimit) return 0.15f;

    float t = (maxDev - startLimit) / (stopLimit - startLimit);
    return 1.0f - t * (1.0f - 0.15f);
}

float PikeSquare::GetCohesionIndex() const {
    if (soldiers.empty()) return 1.0f;

    float totalDeviation = 0.0f;
    for (const auto& agent : soldiers) {
        totalDeviation += Vector2Distance(agent.position, agent.targetSlot);
    }
    float avgDeviation = totalDeviation / static_cast<float>(soldiers.size());
    float maxAllowedDev = spacing * 1.5f;
    float cohesion = 1.0f - (avgDeviation / maxAllowedDev);
    if (cohesion < 0.0f) cohesion = 0.0f;
    return cohesion;
}

void PikeSquare::UpdatePressurePropagation(float /*dt*/) {
    // Projdeme všechny zástupy (roty) od zadních řad k čelu
    for (const auto& rota : roty) {
        int depth = static_cast<int>(rota.soldierIndices.size());
        if (depth <= 0) continue;

        // Zadní řada začíná s nulovým kumulovaným tlakem
        int rearSoldierIdx = rota.soldierIndices[depth - 1];
        if (rearSoldierIdx >= 0 && rearSoldierIdx < static_cast<int>(soldiers.size())) {
            soldiers[rearSoldierIdx].pressureForce = 0.0f;
        }

        // Přenos tlaku od zadní řady k čelu
        for (int r = depth - 2; r >= 0; --r) {
            int currIdx = rota.soldierIndices[r];
            int behindIdx = rota.soldierIndices[r + 1];
            if (currIdx < 0 || currIdx >= static_cast<int>(soldiers.size()) ||
                behindIdx < 0 || behindIdx >= static_cast<int>(soldiers.size())) {
                continue;
            }

            float dist = Vector2Distance(soldiers[behindIdx].position, soldiers[currIdx].position);
            // Pokud je voják za ním v těsném kontaktu (méně než 1.35 * spacing)
            if (dist <= spacing * 1.35f) {
                float basePush = 25.0f * stamina;
                if (pikeState == PikeState::CHARGE) basePush *= 1.25f;
                soldiers[currIdx].pressureForce = soldiers[behindIdx].pressureForce + basePush;
            } else {
                // Mezera v zástupu přeruší přenos kinetického tlaku
                soldiers[currIdx].pressureForce = 0.0f;
            }
        }
    }
}

float PikeSquare::GetAverageFrontPressure() const {
    if (roty.empty()) return 0.0f;
    float sumPressure = 0.0f;
    int count = 0;
    for (const auto& rota : roty) {
        if (!rota.soldierIndices.empty()) {
            int frontIdx = rota.soldierIndices[0];
            if (frontIdx >= 0 && frontIdx < static_cast<int>(soldiers.size())) {
                sumPressure += soldiers[frontIdx].pressureForce;
                count++;
            }
        }
    }
    return (count > 0) ? (sumPressure / static_cast<float>(count)) : 0.0f;
}

float PikeSquare::GetLineDeformation() const {
    if (roty.empty()) return 0.0f;
    float maxDev = 0.0f;
    for (const auto& rota : roty) {
        if (!rota.soldierIndices.empty()) {
            int frontIdx = rota.soldierIndices[0];
            if (frontIdx >= 0 && frontIdx < static_cast<int>(soldiers.size())) {
                float dev = fabsf(soldiers[frontIdx].lineDisplacement);
                if (dev > maxDev) maxDev = dev;
            }
        }
    }
    return maxDev;
}

void PikeSquare::UpdateMeleeAndBreakage(float dt, const SpatialHashGrid* grid) {
    if (!grid) return;

    for (auto& agent : soldiers) {
        if (!agent.isAlive) continue;

        // 1. Kontrola zlomení píky pod mechanickým přetížením (průhyb >= 0.50 rad a tlak > 120 N)
        if (agent.weapon == WeaponType::PIKE && !agent.isPikeBroken) {
            if (fabsf(agent.pikeDeflection) >= 0.50f && agent.pressureForce > 120.0f) {
                agent.isPikeBroken = true;
                agent.inMelee = true;
                agent.isPikeInterlocked = false;
                agent.interlockCount = 0;
            }
        }

        // 2. Úder halapartníků (Sweep Strike rozrážející okolní píky soupeře)
        if (agent.weapon == WeaponType::HALBERD || agent.role == Role::NCO) {
            agent.attackCooldown -= dt;
            if (agent.attackCooldown <= 0.0f) {
                agent.attackCooldown = 1.4f;
                grid->QueryRadius(agent.position, spacing * 1.35f, [&](const GridAgentEntry& neighbor, float /*d*/) {
                    if (neighbor.faction != faction) {
                        // Halaparta destabilizuje linii
                    }
                });
            }
        }

        // 3. Boj zblízka (Melee Combat / Katzbalger)
        grid->QueryRadius(agent.position, 18.0f, [&](const GridAgentEntry& neighbor, float /*d*/) {
            if (neighbor.faction != faction) {
                agent.inMelee = true;
            }
        });

        if (agent.inMelee) {
            agent.attackCooldown -= dt;
            if (agent.attackCooldown <= 0.0f) {
                agent.attackCooldown = 1.2f;

                // Vyhodnocení inkasovaného zásahu v boji zblízka
                bool strikeLands = false;
                grid->QueryRadius(agent.position, 18.0f, [&](const GridAgentEntry& neighbor, float /*d*/) {
                    if (neighbor.faction != faction) {
                        strikeLands = true;
                    }
                });

                if (strikeLands) {
                    // Odražení zásahu kyrysem: Kozáci v přední řadě odrazí 75 % úderů
                    bool deflected = false;
                    if (agent.hasCuirass) {
                        int roll = (agent.id * 17 + static_cast<int>(agent.position.y * 10.0f)) % 100;
                        if (roll < 75) deflected = true;
                    }

                    if (!deflected) {
                        agent.health -= 40.0f;
                        if (agent.health <= 0.0f) {
                            agent.health = 0.0f;
                            agent.isAlive = false;
                            agent.velocity = { 0.0f, 0.0f };
                            agent.pressureForce = 0.0f;
                            agent.interlockCount = 0;
                            agent.isPikeInterlocked = false;
                        }
                    }
                }
            }
        }
    }
}

void PikeSquare::UpdateRankReplenishment() {
    for (auto& rota : roty) {
        int depth = static_cast<int>(rota.soldierIndices.size());
        if (depth <= 1) continue;

        // Procházíme zástup od čela: pokud je voják padlý, první živý za ním přebírá jeho místo
        for (int i = 0; i < depth - 1; ++i) {
            int currIdx = rota.soldierIndices[i];
            if (!soldiers[currIdx].isAlive) {
                int alivePos = -1;
                for (int j = i + 1; j < depth; ++j) {
                    if (soldiers[rota.soldierIndices[j]].isAlive) {
                        alivePos = j;
                        break;
                    }
                }
                if (alivePos != -1) {
                    std::swap(rota.soldierIndices[i], rota.soldierIndices[alivePos]);
                }
            }
        }
    }
    UpdateTargetSlots();
}

int PikeSquare::GetCasualtiesCount() const {
    int count = 0;
    for (const auto& a : soldiers) {
        if (!a.isAlive) count++;
    }
    return count;
}

int PikeSquare::GetLivingCount() const {
    int count = 0;
    for (const auto& a : soldiers) {
        if (a.isAlive) count++;
    }
    return count;
}

int PikeSquare::GetBrokenPikesCount() const {
    int count = 0;
    for (const auto& a : soldiers) {
        if (a.isPikeBroken) count++;
    }
    return count;
}

void PikeSquare::Move(Vector2 dir, float dt, float speed) {
    if (Vector2Length(dir) > 0.01f) {
        hasWaypoint = false;
        hasTargetHeading = false;
        Vector2 normalizedDir = MathUtils::SafeNormalize(dir);
        position = Vector2Add(position, Vector2Scale(normalizedDir, speed * dt));
    }
}

void PikeSquare::Wheel(float dAngle, bool turnRight, float /*dt*/) {
    hasWaypoint = false;
    hasTargetHeading = false;
    float colOffset = (cols - 1) / 2.0f;
    float rowOffset = (rows - 1) / 2.0f;
    Vector2 localPivot = turnRight
        ? Vector2{  colOffset * spacing, -rowOffset * spacing }
        : Vector2{ -colOffset * spacing, -rowOffset * spacing };
    RotateAroundLocalPoint(dAngle, localPivot);
}

void PikeSquare::InPlaceTurn90(bool clockwise) {
    hasWaypoint = false;
    hasTargetHeading = false;

    if (!clockwise) {
        // Otáčení CCW (doleva) o 90 stupňů kolem geometrického středu
        RotateAroundLocalPoint(-PI / 2.0f, { 0.0f, 0.0f });
        angle = MathUtils::NormalizeAngle(angle);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                int new_idx = r * cols + c;
                int old_idx = (rows - 1 - c) * cols + r;
                m_turnBuffer[new_idx] = soldiers[old_idx];
            }
        }
        for (size_t i = 0; i < m_turnBuffer.size(); ++i) {
            soldiers[i] = m_turnBuffer[i];
        }

        // Permutace NCOs (100=FL, 101=FR, 102=BL, 103=BR)
        Agent oldFL = soldiers[100];
        Agent oldFR = soldiers[101];
        Agent oldBL = soldiers[102];
        Agent oldBR = soldiers[103];

        soldiers[100] = oldBL;
        soldiers[101] = oldFL;
        soldiers[102] = oldBR;
        soldiers[103] = oldFR;
    } else {
        // Otáčení CW (doprava) o 90 stupňů kolem geometrického středu
        RotateAroundLocalPoint(PI / 2.0f, { 0.0f, 0.0f });
        angle = MathUtils::NormalizeAngle(angle);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                int new_idx = r * cols + c;
                int old_idx = c * cols + (cols - 1 - r);
                m_turnBuffer[new_idx] = soldiers[old_idx];
            }
        }
        for (size_t i = 0; i < m_turnBuffer.size(); ++i) {
            soldiers[i] = m_turnBuffer[i];
        }

        // Permutace NCOs
        Agent oldFL = soldiers[100];
        Agent oldFR = soldiers[101];
        Agent oldBL = soldiers[102];
        Agent oldBR = soldiers[103];

        soldiers[100] = oldFR;
        soldiers[101] = oldBR;
        soldiers[102] = oldFL;
        soldiers[103] = oldBL;
    }

    angle = MathUtils::NormalizeAngle(angle);
    UpdateTargetSlots();
}

void PikeSquare::SetWaypoint(Vector2 point) {
    targetPos = point;
    hasWaypoint = true;
    hasTargetHeading = false;
}

void PikeSquare::SetOrder(Vector2 point, float finalHeading, bool withHeading) {
    targetPos = point;
    hasWaypoint = true;
    hasTargetHeading = withHeading;
    targetHeading = MathUtils::NormalizeAngle(finalHeading);
}

void PikeSquare::ClearWaypoint() {
    hasWaypoint = false;
    hasTargetHeading = false;
}

void PikeSquare::Update(float dt, const SpatialHashGrid* grid) {
    float tempoFactor = GetNcoTempoFactor();

    // 1. Řízení pohybu (rychlost ovlivněná tempoFactorem)
    float baseSpeed = (pikeState == PikeState::PORTE) ? 80.0f : 45.0f;
    float speed = baseSpeed * tempoFactor;
    float rotationSpeed = 0.8f * tempoFactor;

    float colOffset = (cols - 1) / 2.0f;
    float rowOffset = (rows - 1) / 2.0f;

    Vector2 moveDir = { 0.0f, 0.0f };

    // Dvoufázové navádění k waypointu
    if (hasWaypoint) {
        Vector2 frontCenter = GetFrontCenter();
        Vector2 toWaypoint = Vector2Subtract(targetPos, frontCenter);
        float dist = Vector2Length(toWaypoint);

        if (dist > 5.0f) {
            Vector2 dir = MathUtils::SafeNormalize(toWaypoint);
            float targetAngleNew = atan2f(dir.y, dir.x);
            float angleDiff = MathUtils::NormalizeAngle(targetAngleNew - angle);

            bool isAligned = fabsf(angleDiff) < 0.03f;

            if (!isAligned) {
                // Fáze 1: Otáčení na místě kolem předního rohu
                float maxRotation = rotationSpeed * dt;
                float dAngle = 0.0f;
                Vector2 localPivot;

                if (angleDiff < 0.0f) {
                    dAngle = fmaxf(-maxRotation, angleDiff);
                    localPivot = { -colOffset * spacing, -rowOffset * spacing };
                } else {
                    dAngle = fminf(maxRotation, angleDiff);
                    localPivot = { colOffset * spacing, -rowOffset * spacing };
                }
                RotateAroundLocalPoint(dAngle, localPivot);
            } else {
                // Fáze 2: Pochod vpřed se směrovou mikrokorekcí
                moveDir.x += cosf(angle);
                moveDir.y += sinf(angle);

                float dAngle = angleDiff * 2.0f * dt;
                Vector2 localPivot = { 0.0f, -rowOffset * spacing };
                RotateAroundLocalPoint(dAngle, localPivot);
            }
        } else {
            hasWaypoint = false;
        }
    }

    if (hasTargetHeading && !hasWaypoint) {
        // Fáze 3: Srovnání čela do cílového úhlu (targetHeading) kolem středu formace
        float angleDiff = MathUtils::NormalizeAngle(targetHeading - angle);
        if (fabsf(angleDiff) > 0.03f) {
            float maxRotation = rotationSpeed * dt;
            float dAngle = (angleDiff > 0.0f) ? fminf(maxRotation, angleDiff) : fmaxf(-maxRotation, angleDiff);
            RotateAroundLocalPoint(dAngle, { 0.0f, 0.0f });
        } else {
            hasTargetHeading = false;
        }
    }

    // Translační pohyb formace
    if (Vector2Length(moveDir) > 0.01f) {
        Vector2 normalizedMove = MathUtils::SafeNormalize(moveDir);
        position = Vector2Add(position, Vector2Scale(normalizedMove, speed * dt));
    }

    angle = MathUtils::NormalizeAngle(angle);

    // Přepočet cílových slotů vojáků
    UpdateTargetSlots();

    // Push of Pikes: přenos kinetického tlaku od zadních řad k čelu formace
    UpdatePressurePropagation(dt);
    m_accumulatedPushPressure = GetAverageFrontPressure();

    int activePushContacts = 0;
    float totalDeltaPressure = 0.0f;
    Vector2 forwardDir = { cosf(angle), sinf(angle) };

    // 2. Fyzika a chování jednotlivých agentů (pružnost slotu, repulze, tlumení, piky)
    for (auto& agent : soldiers) {
        if (!agent.isAlive) {
            agent.velocity = { 0.0f, 0.0f };
            agent.pressureForce = 0.0f;
            agent.interlockCount = 0;
            agent.isPikeInterlocked = false;
            continue;
        }

        Vector2 toSlot = Vector2Subtract(agent.targetSlot, agent.position);
        float distToSlot = Vector2Length(toSlot);
        Vector2 springForce = { 0.0f, 0.0f };

        if (distToSlot > 0.1f) {
            float k = 6.0f;
            springForce = Vector2Scale(MathUtils::SafeNormalize(toSlot), distToSlot * k);
        }

        Vector2 separationForce = { 0.0f, 0.0f };
        float personalRadius = spacing * 0.75f;

        if (grid) {
            // O(N) Spatial Query napříč všemi registrovanými formacemi
            grid->QueryRadius(agent.position, personalRadius, [&](const GridAgentEntry& neighbor, float dist) {
                if (neighbor.agentId == agent.id && neighbor.formationId == formationId) return;

                float push = (personalRadius - dist) / personalRadius;
                // Zvýšený odpor linie při střetu s nepřítelem (400 vs 300)
                float factor = (neighbor.faction != faction) ? 400.0f : 300.0f;
                Vector2 toAgent = Vector2Subtract(agent.position, neighbor.position);
                separationForce = Vector2Add(separationForce, Vector2Scale(MathUtils::SafeNormalize(toAgent), push * factor));

                // Push of Pikes: vyhodnocení přetlaku při střetu těl nepřátel v postoji CHARGE
                if (pikeState == PikeState::CHARGE && neighbor.faction != faction && (agent.pressureForce > 0.0f || neighbor.pressureForce > 0.0f)) {
                    activePushContacts++;
                    float deltaP = agent.pressureForce - neighbor.pressureForce;
                    totalDeltaPressure += deltaP;
                    separationForce = Vector2Add(separationForce, Vector2Scale(forwardDir, deltaP * 1.5f));
                }
            });
        } else {
            // Původní fallback pro samostatný izolovaný čtverec
            for (const auto& other : soldiers) {
                if (agent.id == other.id) continue;
                Vector2 toAgent = Vector2Subtract(agent.position, other.position);
                float dist = Vector2Length(toAgent);
                if (dist < personalRadius && dist > 0.01f) {
                    float push = (personalRadius - dist) / personalRadius;
                    float factor = 300.0f;
                    separationForce = Vector2Add(separationForce, Vector2Scale(MathUtils::SafeNormalize(toAgent), push * factor));
                }
            }
        }

        // Výpočet kolizí a brzdné síly pík (Push of Pikes: Interlocking)
        int interlocks = 0;
        float deflectionTorque = 0.0f;
        Vector2 brakingForce = { 0.0f, 0.0f };

        if (grid && pikeState == PikeState::CHARGE && agent.weapon == WeaponType::PIKE && !agent.isPikeBroken) {
            float pikeLength = spacing * 3.2f;
            Vector2 pikeBase = agent.position;
            Vector2 pikeDir = { cosf(agent.pikeAngle), sinf(agent.pikeAngle) };
            Vector2 pikeTip = Vector2Add(pikeBase, Vector2Scale(pikeDir, pikeLength));

            grid->QueryPikeIntersections(agent.id, formationId, faction, pikeBase, pikeTip,
                [&](const GridPikeEntry& otherPike, Vector2 /*hitPt*/, float tSelf, float /*tOther*/) {
                    if (otherPike.faction != faction) {
                        interlocks++;

                        // Směr vzájemného křížení: dSelf x dOther
                        Vector2 dSelf = Vector2Subtract(pikeTip, pikeBase);
                        Vector2 dOther = Vector2Subtract(otherPike.end, otherPike.start);
                        float cross = dSelf.x * dOther.y - dSelf.y * dOther.x;
                        float sign = (cross >= 0.0f) ? 1.0f : -1.0f;

                        // Pákový moment: kontakt blíže k bázi (menší tSelf) působí větší torzi
                        float leverFactor = 1.0f - (tSelf * 0.4f);
                        float torqueMag = 35.0f * leverFactor;
                        deflectionTorque += sign * torqueMag;

                        // Axiální brzdná síla: odpor proti dopřednému postupu formace
                        Vector2 forwardDir = { cosf(agent.angle), sinf(agent.angle) };
                        float forwardVel = Vector2DotProduct(agent.velocity, forwardDir);
                        if (forwardVel > 0.0f) {
                            float brakeMag = fminf(forwardVel * 120.0f, 220.0f);
                            brakingForce = Vector2Subtract(brakingForce, Vector2Scale(forwardDir, brakeMag));
                        }
                    }
                });
        }

        Vector2 totalForce = Vector2Add(Vector2Add(springForce, separationForce), brakingForce);
        float forceMag = Vector2Length(totalForce);
        if (forceMag > agent.maxForce) {
            totalForce = Vector2Scale(MathUtils::SafeNormalize(totalForce), agent.maxForce);
        }

        // Integrace rychlosti
        Vector2 acceleration = Vector2Scale(totalForce, 1.0f / agent.mass);
        agent.velocity = Vector2Add(agent.velocity, Vector2Scale(acceleration, dt));

        // Rychlostní rozpočet (maximální povolená rychlost)
        float maxAllowedSpeed = speed * 1.5f + rotationSpeed * 160.0f;
        maxAllowedSpeed = fmaxf(maxAllowedSpeed, 60.0f);

        float speedCurrent = Vector2Length(agent.velocity);
        if (speedCurrent > maxAllowedSpeed) {
            agent.velocity = Vector2Scale(MathUtils::SafeNormalize(agent.velocity), maxAllowedSpeed);
        }

        // Tlumení třením
        agent.velocity = Vector2Scale(agent.velocity, 0.85f);
        agent.position = Vector2Add(agent.position, Vector2Scale(agent.velocity, dt));

        // Výpočet posunu čela vůči nominálnímu slotu (lineDisplacement)
        Vector2 dispVec = Vector2Subtract(agent.position, agent.targetSlot);
        agent.lineDisplacement = Vector2DotProduct(dispVec, forwardDir);

        // Plynulé otáčení těla k úhlu formace
        float angleDiff = MathUtils::NormalizeAngle(angle - agent.angle);
        float rotFactor = fminf(fmaxf(dt * 5.0f, 0.0f), 1.0f);
        agent.angle += angleDiff * rotFactor;
        agent.angle = MathUtils::NormalizeAngle(agent.angle);

        // 3. Fyzikální houpání za chůze a torzní soustava pík
        float agentSpeed = Vector2Length(agent.velocity);
        if (agentSpeed > 5.0f) {
            agent.swingTimer += agentSpeed * 0.1f * dt;
        } else {
            float lerpFactor = fminf(fmaxf(dt * 5.0f, 0.0f), 1.0f);
            agent.swingTimer = Lerp(agent.swingTimer, 0.0f, lerpFactor);
        }

        float swayAmplitude = (pikeState == PikeState::PORTE) ? 0.06f : 0.12f;
        float sway = sinf(agent.swingTimer) * swayAmplitude;

        if (pikeState == PikeState::PORTE) {
            agent.interlockCount = 0;
            agent.isPikeInterlocked = false;
            agent.pikeDeflection = 0.0f;
            agent.pikeAngularVelocity = 0.0f;
            agent.pikeAngle = MathUtils::NormalizeAngle(agent.angle - 0.1f + sway);
        } else if (agent.weapon == WeaponType::PIKE) {
            // Taktický stav CHARGE s píkami: torzní pružina s dynamickým odklonem
            float targetPikeAngle = MathUtils::NormalizeAngle(agent.angle + sway);
            float springError = MathUtils::NormalizeAngle(targetPikeAngle - agent.pikeAngle);
            float springTorque = springError * 35.0f; // k_tau = 35.0

            // Celková úhlová akcelerace: torzní pružina + odklon při kontaktu - tlumení
            float totalAngAcc = springTorque + deflectionTorque - agent.pikeAngularVelocity * 6.0f; // c_tau = 6.0
            agent.pikeAngularVelocity += totalAngAcc * dt;
            agent.pikeAngle += agent.pikeAngularVelocity * dt;

            // Omezení maximálního průhybu na +-0.6 rad od klidového směru
            float currentDeflection = MathUtils::NormalizeAngle(agent.pikeAngle - targetPikeAngle);
            const float MAX_DEFLECTION = 0.6f;
            if (currentDeflection > MAX_DEFLECTION) {
                agent.pikeAngle = MathUtils::NormalizeAngle(targetPikeAngle + MAX_DEFLECTION);
                agent.pikeAngularVelocity = 0.0f;
                currentDeflection = MAX_DEFLECTION;
            } else if (currentDeflection < -MAX_DEFLECTION) {
                agent.pikeAngle = MathUtils::NormalizeAngle(targetPikeAngle - MAX_DEFLECTION);
                agent.pikeAngularVelocity = 0.0f;
                currentDeflection = -MAX_DEFLECTION;
            }

            agent.pikeAngle = MathUtils::NormalizeAngle(agent.pikeAngle);
            agent.pikeDeflection = currentDeflection;
            agent.interlockCount = interlocks;
            agent.isPikeInterlocked = (interlocks > 0);
        } else {
            agent.interlockCount = 0;
            agent.isPikeInterlocked = false;
            agent.pikeDeflection = 0.0f;
            agent.pikeAngularVelocity = 0.0f;
            agent.pikeAngle = MathUtils::NormalizeAngle(agent.angle + sway);
        }
    }

    // 4. Push of Pikes: vyhodnocení stavu přetlačování (Push Contest)
    if (activePushContacts > 0) {
        m_isPushing = true;
        // Odčerpávání staminy během fyzického střetu tělo na tělo
        stamina = fmaxf(0.0f, stamina - 0.015f * dt);

        float avgDeltaP = totalDeltaPressure / static_cast<float>(activePushContacts);
        if (fabsf(avgDeltaP) > 20.0f) {
            float pushVelocity = fminf(fmaxf(avgDeltaP * 0.08f, -14.0f), 14.0f);
            position = Vector2Add(position, Vector2Scale(forwardDir, pushVelocity * dt));
        }

        m_lineDeformation = GetLineDeformation();
        // Pokud je čelo zatlačeno zpět o více než mezní hodnotu, nastává postih morálky (Breach)
        if (m_lineDeformation > 35.0f) {
            morale = fmaxf(0.0f, morale - 0.05f * dt);
        }
    } else {
        m_isPushing = false;
        m_lineDeformation = 0.0f;
        // Pozvolná regenerace staminy mimo boj
        stamina = fminf(1.0f, stamina + 0.005f * dt);
    }

    // 5. Bad War: zlomení pík pod tlakem, boj zblízka a doplňování padlých (gravitace zástupu)
    UpdateMeleeAndBreakage(dt, grid);
    UpdateRankReplenishment();
}
