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
        agent.position = position;
        agent.velocity = { 0.0f, 0.0f };
        agent.angle = angle;
        agent.pikeAngle = angle;
        agent.swingTimer = static_cast<float>(GetRandomValue(0, 100)) / 10.0f;
        soldiers.push_back(agent);
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
    float colOffset = (cols - 1) / 2.0f;
    float rowOffset = (rows - 1) / 2.0f;
    int index = 0;
    float rotAngle = angle + PI / 2.0f;

    // 1. Sloty pro řadové pikenýry (kompaktní mřížka)
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            float localX = (col - colOffset) * spacing;
            float localY = (row - rowOffset) * spacing;

            Vector2 rotatedOffset = MathUtils::RotateVector({ localX, localY }, rotAngle);
            soldiers[index].targetSlot = Vector2Add(position, rotatedOffset);
            index++;
        }
    }

    // 2. Sloty pro 4 Poddůstojníky (NCOs) - diagonálně vně rohů
    float ncoOutFactor = 1.0f;
    float cornerOffsets[4][2] = {
        { -colOffset - ncoOutFactor, -rowOffset - ncoOutFactor }, // Přední levý (FL)
        {  colOffset + ncoOutFactor, -rowOffset - ncoOutFactor }, // Přední pravý (FR)
        { -colOffset - ncoOutFactor,  rowOffset + ncoOutFactor }, // Zadní levý (BL)
        {  colOffset + ncoOutFactor,  rowOffset + ncoOutFactor }  // Zadní pravý (BR)
    };

    for (int i = 0; i < 4; ++i) {
        float localX = cornerOffsets[i][0] * spacing;
        float localY = cornerOffsets[i][1] * spacing;

        Vector2 rotatedOffset = MathUtils::RotateVector({ localX, localY }, rotAngle);
        soldiers[index].targetSlot = Vector2Add(position, rotatedOffset);
        index++;
    }

    // 3. Slot pro Velitele (Leader) - bezpečně za zadní řadou uprostřed
    {
        float localX = 0.0f;
        float localY = (rowOffset + 1.4f) * spacing;

        Vector2 rotatedOffset = MathUtils::RotateVector({ localX, localY }, rotAngle);
        soldiers[index].targetSlot = Vector2Add(position, rotatedOffset);
    }
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

    bool manualInput = (faction == Faction::PLAYER) && (IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D));
    if (manualInput) {
        hasWaypoint = false;
        hasTargetHeading = false;
    }

    if (faction == Faction::PLAYER) {
        if (IsKeyDown(KEY_W)) {
            moveDir.x += cosf(angle);
            moveDir.y += sinf(angle);
        }
        if (IsKeyDown(KEY_S)) {
            moveDir.x -= cosf(angle);
            moveDir.y -= sinf(angle);
        }

        bool shiftPressed = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        if (shiftPressed) {
            if (IsKeyPressed(KEY_A)) {
                InPlaceTurn90(false);
            }
            if (IsKeyPressed(KEY_D)) {
                InPlaceTurn90(true);
            }
        } else {
            if (IsKeyDown(KEY_A)) {
                float dAngle = -rotationSpeed * dt;
                Vector2 localPivot = { -colOffset * spacing, -rowOffset * spacing };
                RotateAroundLocalPoint(dAngle, localPivot);
            }
            if (IsKeyDown(KEY_D)) {
                float dAngle = rotationSpeed * dt;
                Vector2 localPivot = { colOffset * spacing, -rowOffset * spacing };
                RotateAroundLocalPoint(dAngle, localPivot);
            }
        }
    }

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

    // 2. Fyzika a chování jednotlivých agentů (pružnost slotu, repulze, tlumení)
    for (auto& agent : soldiers) {
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

        Vector2 totalForce = Vector2Add(springForce, separationForce);
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

        // Plynulé otáčení těla k úhlu formace
        float angleDiff = MathUtils::NormalizeAngle(angle - agent.angle);
        float rotFactor = fminf(fmaxf(dt * 5.0f, 0.0f), 1.0f);
        agent.angle += angleDiff * rotFactor;
        agent.angle = MathUtils::NormalizeAngle(agent.angle);

        // 3. Fyzikální houpání píky za chůze
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
            agent.pikeAngle = agent.angle - 0.1f + sway;
        } else {
            agent.pikeAngle = agent.angle + sway;
        }
    }
}
