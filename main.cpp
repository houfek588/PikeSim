#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <string>

// --- KONSTANTY A NASTAVENÍ ---
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;

enum class Role {
    PIKEMAN,    // Řadový pikenýr (tvoří plné jádru formace)
    NCO,        // Poddůstojník (hlídá rohy vně formace a určuje tempo)
    LEADER      // Kapitán / Velitel (řídí jednotku zezadu)
};

enum class PikeState {
    PORTE,      // Píky nahoře (rychlejší přesun)
    CHARGE      // Píky skloněné dopředu (pomalý bojový pochod)
};

enum class VisualMode {
    ENGRAVING,  // Styl staré papírové rytiny
    VECTOR      // Minimalistický vektorový režim
};

// --- STRUKTURA PRO JEDNOTLIVÉHO VOJÁKA ---
struct Agent {
    int id;
    Role role;
    Vector2 position;
    Vector2 velocity;
    Vector2 targetSlot;   // Ideální pozice v mřížce formace
    float angle;          // Směr pohledu/těla
    float pikeAngle;      // Směr píky
    float swingTimer;     // Časovač pro kymácení píky při chůzi
    float mass;           // Hmotnost (ovlivňuje setrvačnost)
    float maxForce;       // Maximální síla, kterou může vyvinout k dosažení slotu
};

// Vlastní implementace přerušované čáry, kterou Raylib nemá v základu
void DrawLineDashed(Vector2 startPos, Vector2 endPos, float dashLength, float spaceLength, Color color) {
    Vector2 diff = Vector2Subtract(endPos, startPos);
    float len = Vector2Length(diff);
    if (len <= 0.0f) return;
    Vector2 dir = Vector2Normalize(diff);
    float currentLength = 0.0f;
    while (currentLength < len) {
        float drawLength = fminf(dashLength, len - currentLength);
        Vector2 dashStart = Vector2Add(startPos, Vector2Scale(dir, currentLength));
        Vector2 dashEnd = Vector2Add(dashStart, Vector2Scale(dir, drawLength));
        DrawLineV(dashStart, dashEnd, color);
        currentLength += dashLength + spaceLength;
    }
}

// --- TŘÍDA PRO PIKENÝRSKÝ ČTVEREC ---
class PikeSquare {
public:
    Vector2 position;       // Střed formace (pivot)
    float angle;            // Úhel rotace formace (v radiánech)
    float targetAngle;      // Cílový úhel rotace
    Vector2 targetPos;      // Cílová pozice (waypoint)
    bool hasWaypoint;

    int cols;
    int rows;
    float spacing;          // Rozestupy mezi vojáky v pixelech
    PikeState pikeState;
    std::vector<Agent> soldiers;

    PikeSquare(float x, float y, int c = 10, int r = 10, float space = 26.0f) {
        position = { x, y };
        targetPos = position;
        angle = -PI / 2.0f; // Výchozí pohled směrem nahoru
        targetAngle = angle;
        hasWaypoint = false;
        cols = c;
        rows = r;
        spacing = space;
        pikeState = PikeState::PORTE;

        int idCounter = 0;

        // 1. INICIALIZACE PLNÉHO BLOKU PIKENÝRŮ (bez děr v rozích a vzadu)
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                Agent agent;
                agent.id = idCounter++;
                agent.role = Role::PIKEMAN;
                agent.mass = 1.0f;
                agent.maxForce = 180.0f;
                agent.position = position;
                agent.velocity = { 0, 0 };
                agent.angle = angle;
                agent.pikeAngle = angle;
                agent.swingTimer = (float)GetRandomValue(0, 100) / 10.0f;
                soldiers.push_back(agent);
            }
        }

        // 2. PŘIDÁNÍ PODDŮSTOJNÍKŮ (NCOs) jako dodatečných externích jednotek (4 rohy)
        for (int i = 0; i < 4; ++i) {
            Agent agent;
            agent.id = idCounter++;
            agent.role = Role::NCO;
            agent.mass = 1.0f;       // Stejná setrvačnost jako u pikenýrů
            agent.maxForce = 180.0f; // Stejná maximální síla jako pikenýři pro sjednocení setrvačnosti
            agent.position = position;
            agent.velocity = { 0, 0 };
            agent.angle = angle;
            agent.pikeAngle = angle;
            agent.swingTimer = (float)GetRandomValue(0, 100) / 10.0f;
            soldiers.push_back(agent);
        }

        // 3. PŘIDÁNÍ VELITELE (Leader) jako dodatečné externí jednotky (za zadní řadou)
        {
            Agent agent;
            agent.id = idCounter++;
            agent.role = Role::LEADER;
            agent.mass = 1.0f;       // Stejná setrvačnost jako u pikenýrů
            agent.maxForce = 180.0f; // Stejná maximální síla jako pikenýři
            agent.position = position;
            agent.velocity = { 0, 0 };
            agent.angle = angle;
            agent.pikeAngle = angle;
            agent.swingTimer = (float)GetRandomValue(0, 100) / 10.0f;
            soldiers.push_back(agent);
        }

        // První inicializace cílových pozic
        UpdateTargetSlots();
        for (auto& agent : soldiers) {
            agent.position = agent.targetSlot;
        }
    }

    // Pomocná metoda pro výpočet globální pozice středu přední řady (Front-Center)
    Vector2 GetFrontCenter() const {
        float rowOffset = (rows - 1) / 2.0f;
        float frontLocalY = -rowOffset * spacing;
        float rotAngle = angle + PI / 2.0f; // Sladění rotace s pohledem pikenýrů
        Vector2 rotatedOffset = {
            localYRotatedX(0, frontLocalY, rotAngle),
            localYRotatedY(0, frontLocalY, rotAngle)
        };
        return Vector2Add(position, rotatedOffset);
    }

    // Pomocné inline metody pro transformaci souřadnic s offsetem
    inline float localYRotatedX(float lx, float ly, float rAngle) const {
        return lx * cosf(rAngle) - ly * sinf(rAngle);
    }
    inline float localYRotatedY(float lx, float ly, float rAngle) const {
        return lx * sinf(rAngle) + ly * cosf(rAngle);
    }

    // Metoda pro rotaci formace kolem libovolného lokálního bodu (pivotu) bez posunu tohoto bodu v globálním prostoru
    void RotateAroundLocalPoint(float dAngle, Vector2 localPivot) {
        float rotAngleOld = angle + PI / 2.0f;
        Vector2 rotatedOffsetOld = {
            localPivot.x * cosf(rotAngleOld) - localPivot.y * sinf(rotAngleOld),
            localPivot.x * sinf(rotAngleOld) + localPivot.y * cosf(rotAngleOld)
        };
        Vector2 globalPivot = Vector2Add(position, rotatedOffsetOld);

        angle += dAngle;

        float rotAngleNew = angle + PI / 2.0f;
        Vector2 rotatedOffsetNew = {
            localPivot.x * cosf(rotAngleNew) - localPivot.y * sinf(rotAngleNew),
            localPivot.x * sinf(rotAngleNew) + localPivot.y * cosf(rotAngleNew)
        };
        position = Vector2Subtract(globalPivot, rotatedOffsetNew);
    }

    // Výpočet ideálních pozic mřížky na základě aktuálního středu a rotace
    void UpdateTargetSlots() {
        float colOffset = (cols - 1) / 2.0f;
        float rowOffset = (rows - 1) / 2.0f;
        int index = 0;
        float rotAngle = angle + PI / 2.0f; // Sladění rotace s pohledem pikenýrů

        // 1. Sloty pro 100 řadových pikenýrů (kompaktní čtverec)
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                float localX = (col - colOffset) * spacing;
                float localY = (row - rowOffset) * spacing;

                Vector2 rotatedOffset = {
                    localX * cosf(rotAngle) - localY * sinf(rotAngle),
                    localX * sinf(rotAngle) + localY * cosf(rotAngle)
                };

                soldiers[index].targetSlot = Vector2Add(position, rotatedOffset);
                index++;
            }
        }

        // 2. Sloty pro 4 Poddůstojníky (NCOs) - vysunuti diagonálně vně rohů kompaktního bloku
        float ncoOutFactor = 1.0f; // Vzdálenost diagonálně ven od rohů
        float cornerOffsets[4][2] = {
            {-colOffset - ncoOutFactor, -rowOffset - ncoOutFactor}, // Přední levý roh
            {colOffset + ncoOutFactor, -rowOffset - ncoOutFactor},  // Přední pravý roh
            {-colOffset - ncoOutFactor, rowOffset + ncoOutFactor},  // Zadní levý roh
            {colOffset + ncoOutFactor, rowOffset + ncoOutFactor}    // Zadní pravý roh
        };

        for (int i = 0; i < 4; ++i) {
            float localX = cornerOffsets[i][0] * spacing;
            float localY = cornerOffsets[i][1] * spacing;

            Vector2 rotatedOffset = {
                localX * cosf(rotAngle) - localY * sinf(rotAngle),
                localX * sinf(rotAngle) + localY * cosf(rotAngle)
            };

            soldiers[index].targetSlot = Vector2Add(position, rotatedOffset);
            index++;
        }

        // 3. Slot pro Velitele (Leader) - tlačen bezpečně doprostřed za zadní řadu
        {
            float localX = 0.0f; // Horizontálně uprostřed
            float localY = (rowOffset + 1.4f) * spacing; // Posunut za zadní řadu

            Vector2 rotatedOffset = {
                localX * cosf(rotAngle) - localY * sinf(rotAngle),
                localX * sinf(rotAngle) + localY * cosf(rotAngle)
            };

            soldiers[index].targetSlot = Vector2Add(position, rotatedOffset);
        }
    }

    // Výpočet vlivu poddůstojníků na tempo formace (NCOs určují rychlost čtverce)
    float GetNcoTempoFactor() const {
        float maxDev = 0.0f;

        // Zjistíme, jak moc se poddůstojníci (NCOs) odchýlili od svých určených rohů
        for (const auto& agent : soldiers) {
            if (agent.role == Role::NCO) {
                float dist = Vector2Distance(agent.position, agent.targetSlot);
                if (dist > maxDev) {
                    maxDev = dist;
                }
            }
        }

        // Hranice pro snížení tempa jsou o něco benevolentnější, aby formace plynule reagovala
        float startLimit = spacing * 0.40f; // Začátek zpomalování při mírném zaostání
        float stopLimit = spacing * 2.0f;   // Limit, kdy formace téměř zastaví a čeká na srovnání

        if (maxDev <= startLimit) return 1.0f;
        if (maxDev >= stopLimit) return 0.15f; // Minimální nouzové tempo (15 %)

        float t = (maxDev - startLimit) / (stopLimit - startLimit);
        return 1.0f - t * (1.0f - 0.15f);
    }

    void Update(float dt) {
        float tempoFactor = GetNcoTempoFactor();

        // 1. Řízení pohybu (rychlost je ovlivněna tempoFactorem)
        float baseSpeed = (pikeState == PikeState::PORTE) ? 80.0f : 45.0f;
        float speed = baseSpeed * tempoFactor;
        float rotationSpeed = 0.8f * tempoFactor;

        float colOffset = (cols - 1) / 2.0f;
        float rowOffset = (rows - 1) / 2.0f;

        Vector2 moveDir = { 0, 0 };

        if (IsKeyDown(KEY_W)) {
            moveDir.x += cosf(angle);
            moveDir.y += sinf(angle);
        }
        if (IsKeyDown(KEY_S)) {
            moveDir.x -= cosf(angle);
            moveDir.y -= sinf(angle);
        }

        // Detekce stisku klávesy Shift
        bool shiftPressed = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        if (shiftPressed) {
            // --- SHIFT + A / D - INDIVIDUÁLNÍ OBRAT NA MÍSTĚ O 90° (Změna čela formace) ---
            if (IsKeyPressed(KEY_A)) {
                hasWaypoint = false; // Zrušíme případný waypoint pochodu

                // 1. Natočíme celou formaci doleva o 90 stupňů kolem geometrického středu
                RotateAroundLocalPoint(-PI / 2.0f, { 0.0f, 0.0f });

                // 2. Provedeme maticovou rotaci pikenýrů v poli (0 až 99) proti směru hodinových ručiček.
                std::vector<Agent> tempPikemen(cols * rows);
                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        int new_idx = r * cols + c;
                        int old_idx = (rows - 1 - c) * cols + r; // Přesný matematický CCW převod
                        tempPikemen[new_idx] = soldiers[old_idx];
                    }
                }
                for (size_t i = 0; i < tempPikemen.size(); ++i) {
                    soldiers[i] = tempPikemen[i];
                }

                // 3. Permutujeme poddůstojníky (NCOs) na rozích, aby také zůstali stát na místě
                // NCOs indexy: 100 = FL, 101 = FR, 102 = BL, 103 = BR
                Agent oldFL = soldiers[100];
                Agent oldFR = soldiers[101];
                Agent oldBL = soldiers[102];
                Agent oldBR = soldiers[103];

                // Při otočení CCW (Doleva):
                // Nový FL (100) byl fyzicky na BL (102)
                // Nový FR (101) byl fyzicky na FL (100)
                // Nový BL (102) byl fyzicky na BR (103)
                // Nový BR (103) byl fyzicky na FR (101)
                soldiers[100] = oldBL;
                soldiers[101] = oldFL;
                soldiers[102] = oldBR;
                soldiers[103] = oldFR;
            }
            if (IsKeyPressed(KEY_D)) {
                hasWaypoint = false; // Zrušíme případný waypoint pochodu

                // 1. Natočíme celou formaci doprava o 90 stupňů kolem geometrického středu
                RotateAroundLocalPoint(PI / 2.0f, { 0.0f, 0.0f });

                // 2. Provedeme maticovou rotaci pikenýrů v poli (0 až 99) po směru hodinových ručiček.
                std::vector<Agent> tempPikemen(cols * rows);
                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        int new_idx = r * cols + c;
                        int old_idx = c * cols + (cols - 1 - r); // Přesný matematický CW převod
                        tempPikemen[new_idx] = soldiers[old_idx];
                    }
                }
                for (size_t i = 0; i < tempPikemen.size(); ++i) {
                    soldiers[i] = tempPikemen[i];
                }

                // 3. Permutujeme poddůstojníky (NCOs) na rozích, aby také zůstali stát na místě
                Agent oldFL = soldiers[100];
                Agent oldFR = soldiers[101];
                Agent oldBL = soldiers[102];
                Agent oldBR = soldiers[103];

                // Při otočení CW (Doprava):
                // Nový FL (100) byl fyzicky na FR (101)
                // Nový FR (101) byl fyzicky na BR (103)
                // Nový BL (102) byl fyzicky na FL (100)
                // Nový BR (103) byl fyzicky na BL (102)
                soldiers[100] = oldFR;
                soldiers[101] = oldBR;
                soldiers[102] = oldFL;
                soldiers[103] = oldBL;
            }
        } else {
            // A/D - PLNÁ ROTACE (KOLO) KOLEM PŘEDNÍCH ROHŮ
            if (IsKeyDown(KEY_A)) {
                // Točení doleva -> osa otáčení je PŘEDNÍ LEVÝ ROH
                float dAngle = -rotationSpeed * dt;
                Vector2 localPivot = { -colOffset * spacing, -rowOffset * spacing };
                RotateAroundLocalPoint(dAngle, localPivot);
            }
            if (IsKeyDown(KEY_D)) {
                // Točení doprava -> osa otáčení je PŘEDNÍ PRAVÝ ROH
                float dAngle = rotationSpeed * dt;
                Vector2 localPivot = { colOffset * spacing, -rowOffset * spacing };
                RotateAroundLocalPoint(dAngle, localPivot);
            }
        }

        // Dvoufázové ovládání pomocí waypointu: 1. otočení kolem příslušného předního rohu, 2. pochod vpřed jako W
        if (hasWaypoint) {
            Vector2 frontCenter = GetFrontCenter();
            Vector2 toWaypoint = Vector2Subtract(targetPos, frontCenter);
            float dist = Vector2Length(toWaypoint);

            if (dist > 5.0f) {
                Vector2 dir = Vector2Normalize(toWaypoint);
                float targetAngleNew = atan2f(dir.y, dir.x);
                float angleDiff = targetAngleNew - angle;
                while (angleDiff < -PI) angleDiff += 2 * PI;
                while (angleDiff > PI) angleDiff -= 2 * PI;

                // Určení, zda jsme dostatečně otočení (tolerance cca 1.7 stupně pro plynulý přechod do pochodu)
                bool isAligned = fabsf(angleDiff) < 0.03f;

                if (!isAligned) {
                    // FÁZE 1: Pouze otáčení na místě kolem předního rohu (žádný pohyb vpřed)
                    float maxRotation = rotationSpeed * dt;
                    float dAngle = 0.0f;
                    Vector2 localPivot;

                    if (angleDiff < 0.0f) {
                        // Cíl je vlevo -> otáčení doleva kolem PŘEDNÍHO LEVÉHO rohu
                        dAngle = fmaxf(-maxRotation, angleDiff);
                        localPivot = { -colOffset * spacing, -rowOffset * spacing };
                    } else {
                        // Cíl je vpravo -> otáčení doprava kolem PŘEDNÍHO PRAVÉHO rohu
                        dAngle = fminf(maxRotation, angleDiff);
                        localPivot = { colOffset * spacing, -rowOffset * spacing };
                    }
                    RotateAroundLocalPoint(dAngle, localPivot);
                } else {
                    // FÁZE 2: Soudržný pochod vpřed (pohybujeme se podél aktuálního vektoru pohledu čtverce)
                    moveDir.x += cosf(angle);
                    moveDir.y += sinf(angle);

                    // Během pochodu udržujeme jemnou korekci směru (kolem Front-Center pivotu pro přirozenou chůzi)
                    float dAngle = angleDiff * 2.0f * dt;
                    Vector2 localPivot = { 0.0f, -rowOffset * spacing }; // Front-Center
                    RotateAroundLocalPoint(dAngle, localPivot);
                }
            } else {
                hasWaypoint = false;
            }
        }

        if (Vector2Length(moveDir) > 0.01f) {
            moveDir = Vector2Normalize(moveDir);
            position = Vector2Add(position, Vector2Scale(moveDir, speed * dt));
        }

        while (angle < -PI) angle += 2 * PI;
        while (angle > PI) angle -= 2 * PI;

        // Přepočítat cílové sloty vojáků
        UpdateTargetSlots();

        // 2. Fyzika a chování jednotlivých agentů
        for (auto& agent : soldiers) {
            // Síla přitahující vojáka do jeho slotu (pružnost formace)
            Vector2 toSlot = Vector2Subtract(agent.targetSlot, agent.position);
            float distToSlot = Vector2Length(toSlot);
            Vector2 springForce = { 0, 0 };

            if (distToSlot > 0.1f) {
                // Všichni agenti (včetně velitelů) sdílí naprosto shodnou tuhost pružiny k = 6.0f!
                // To zajišťuje, že se pohybují se stejnou kinetickou pružností a setrvačností.
                float k = 6.0f;
                springForce = Vector2Scale(Vector2Normalize(toSlot), distToSlot * k);
            }

            // Repulzní kolizní sféry mezi vojáky (udržování rozestupu)
            Vector2 separationForce = { 0, 0 };
            float personalRadius = spacing * 0.75f;

            for (const auto& other : soldiers) {
                if (agent.id == other.id) continue;
                Vector2 toAgent = Vector2Subtract(agent.position, other.position);
                float dist = Vector2Length(toAgent);
                if (dist < personalRadius && dist > 0.01f) {
                    float push = (personalRadius - dist) / personalRadius;
                    // Sjednoceno na faktor 300.0f pro všechny, aby se zamezilo clippingu a anomáliím v rozích
                    float factor = 300.0f;
                    separationForce = Vector2Add(separationForce, Vector2Scale(Vector2Normalize(toAgent), push * factor));
                }
            }

            // Výsledná síla aplikovaná na agenta
            Vector2 totalForce = Vector2Add(springForce, separationForce);

            // Omezení maximální síly podle sjednocených vlastností
            float forceMag = Vector2Length(totalForce);
            if (forceMag > agent.maxForce) {
                totalForce = Vector2Scale(Vector2Normalize(totalForce), agent.maxForce);
            }

            // Eulerova integrace
            Vector2 acceleration = Vector2Scale(totalForce, 1.0f / agent.mass);
            agent.velocity = Vector2Add(agent.velocity, Vector2Scale(acceleration, dt));

            // BEZPEČNOSTNÍ RYCHLOSTNÍ LIMIT (Rychlostní rozpočet zamezuje "raketovému" efektu a únikům)
            // Zahrnuje translační rychlost formace + rotační poloměr rohů
            float maxAllowedSpeed = speed * 1.5f + rotationSpeed * 160.0f;
            maxAllowedSpeed = fmaxf(maxAllowedSpeed, 60.0f); // Spodní hranice pro bezpečný návrat do slotů, když formace stojí

            float speedCurrent = Vector2Length(agent.velocity);
            if (speedCurrent > maxAllowedSpeed) {
                agent.velocity = Vector2Scale(Vector2Normalize(agent.velocity), maxAllowedSpeed);
            }

            // Tlumení pohybu (tření o zem)
            agent.velocity = Vector2Scale(agent.velocity, 0.85f);
            agent.position = Vector2Add(agent.position, Vector2Scale(agent.velocity, dt));

            // Otáčení vojáka ve směru formace
            float angleDiff = angle - agent.angle;
            while (angleDiff < -PI) angleDiff += 2 * PI;
            while (angleDiff > PI) angleDiff -= 2 * PI;
            agent.angle += angleDiff * 5.0f * dt;

            // 3. Fyzikální houpání píky za chůze
            float agentSpeed = Vector2Length(agent.velocity);
            if (agentSpeed > 5.0f) {
                agent.swingTimer += agentSpeed * 0.1f * dt;
            } else {
                agent.swingTimer = Lerp(agent.swingTimer, 0.0f, 5.0f * dt);
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

    // Výpočet indexu soudržnosti (0.0f = rozpad, 1.0f = dokonalá formace)
    float GetCohesionIndex() const {
        float totalDeviation = 0.0f;
        for (const auto& agent : soldiers) {
            totalDeviation += Vector2Distance(agent.position, agent.targetSlot);
        }
        float avgDeviation = totalDeviation / soldiers.size();
        float maxAllowedDev = spacing * 1.5f;
        float cohesion = 1.0f - (avgDeviation / maxAllowedDev);
        if (cohesion < 0.0f) cohesion = 0.0f;
        return cohesion;
    }
};

// --- FUNKCE PRO VYKRESLENÍ ---
void DrawEngravingAgent(const Agent& agent, float spacing, PikeState state) {
    Color inkDark = { 45, 38, 34, 255 };
    Color inkLight = { 120, 110, 100, 255 };
    Color plumeColor = inkLight;

    if (agent.role == Role::LEADER) plumeColor = { 180, 50, 40, 255 }; // Výrazné červené pero velitele
    else if (agent.role == Role::NCO) plumeColor = { 190, 130, 40, 255 }; // Oranžové pero poddůstojníka

    Vector2 pos = agent.position;
    float r = spacing * 0.35f;

    // Kreslení stínu na papíře
    DrawCircleSector(Vector2Add(pos, {2, 3}), r, 0, 360, 8, {0, 0, 0, 40});

    // Tělo vojáka
    DrawCircleSector(pos, r, 0, 360, 12, inkLight);
    DrawCircleLines(pos.x, pos.y, r, inkDark);

    // Směrová čára ramen
    Vector2 leftShoulder = { pos.x - r * sinf(agent.angle), pos.y + r * cosf(agent.angle) };
    Vector2 rightShoulder = { pos.x + r * sinf(agent.angle), pos.y - r * cosf(agent.angle) };
    DrawLineEx(leftShoulder, rightShoulder, 1.5f, inkDark);

    // Přilba
    DrawCircle(pos.x, pos.y, r * 0.5f, { 220, 220, 220, 255 });
    DrawCircleLines(pos.x, pos.y, r * 0.5f, inkDark);

    // Plume (pero na přilbě)
    Vector2 plumeDir = { -cosf(agent.angle), -sinf(agent.angle) };
    Vector2 plumePos = Vector2Add(pos, Vector2Scale(plumeDir, r * 0.6f));
    DrawCircle(plumePos.x, plumePos.y, r * 0.3f, plumeColor);
    DrawCircleLines(plumePos.x, plumePos.y, r * 0.3f, inkDark);

    // Vykreslení zbraně
    float pikeLength = (state == PikeState::PORTE) ? spacing * 1.8f : spacing * 3.5f;
    Vector2 pikeDir = { cosf(agent.pikeAngle), sinf(agent.pikeAngle) };

    Vector2 weaponStart = Vector2Add(pos, Vector2Scale({ sinf(agent.angle), -cosf(agent.angle) }, r * 0.4f));
    Vector2 weaponEnd = Vector2Add(weaponStart, Vector2Scale(pikeDir, pikeLength));

    DrawLineEx(Vector2Add(weaponStart, {3, 5}), Vector2Add(weaponEnd, {3, 5}), 1.0f, {0, 0, 0, 30});

    if (agent.role == Role::NCO) {
        // Poddůstojník má halapartnu
        float halberdLength = spacing * 1.5f;
        Vector2 halberdEnd = Vector2Add(weaponStart, Vector2Scale(pikeDir, halberdLength));
        DrawLineEx(weaponStart, halberdEnd, 1.8f, { 95, 75, 55, 255 });
        DrawLineEx(weaponStart, halberdEnd, 1.0f, inkDark);
        Vector2 headPos = Vector2Subtract(halberdEnd, Vector2Scale(pikeDir, 3.0f));
        DrawCircleSector(headPos, 4.0f, 0, 360, 4, { 180, 185, 190, 255 });
        DrawCircleSectorLines(headPos, 4.0f, 0, 360, 4, inkDark);
    } else if (agent.role == Role::LEADER) {
        // Velitel má tasený meč
        float swordLength = spacing * 1.1f;
        Vector2 swordEnd = Vector2Add(weaponStart, Vector2Scale(pikeDir, swordLength));
        DrawLineEx(weaponStart, swordEnd, 1.5f, { 200, 200, 205, 255 });
        DrawLineEx(weaponStart, swordEnd, 0.8f, inkDark);
    } else {
        // Klasická dlouhá píka
        DrawLineEx(weaponStart, weaponEnd, 1.5f, { 115, 85, 60, 255 });
        DrawLineEx(weaponStart, weaponEnd, 0.8f, inkDark);

        Vector2 tipBack = Vector2Subtract(weaponEnd, Vector2Scale(pikeDir, 6.0f));
        Vector2 tipLeft = { tipBack.x - 2.5f * sinf(agent.pikeAngle), tipBack.y + 2.5f * cosf(agent.pikeAngle) };
        Vector2 tipRight = { tipBack.x + 2.5f * sinf(agent.pikeAngle), tipBack.y - 2.5f * cosf(agent.pikeAngle) };

        DrawTriangle(weaponEnd, tipLeft, tipRight, { 190, 195, 200, 255 });
        DrawTriangleLines(weaponEnd, tipLeft, tipRight, inkDark);
    }
}

void DrawVectorAgent(const Agent& agent, float spacing, PikeState state) {
    Color coreColor = { 0, 200, 255, 255 };
    if (agent.role == Role::LEADER) coreColor = { 255, 50, 50, 255 };
    else if (agent.role == Role::NCO) coreColor = { 255, 180, 0, 255 };

    Vector2 pos = agent.position;
    float r = spacing * 0.25f;

    DrawCircleLines(agent.targetSlot.x, agent.targetSlot.y, 3.0f, { 100, 100, 100, 100 });
    DrawLineV(pos, agent.targetSlot, { 255, 255, 255, 50 });

    DrawCircleV(pos, r, coreColor);
    DrawCircleLines(pos.x, pos.y, r + 1, { 255, 255, 255, 150 });

    Vector2 velEnd = Vector2Add(pos, Vector2Scale(agent.velocity, 0.15f));
    DrawLineV(pos, velEnd, { 0, 255, 100, 180 });

    float pikeLength = (state == PikeState::PORTE) ? spacing * 1.5f : spacing * 3.2f;
    Vector2 pikeDir = { cosf(agent.pikeAngle), sinf(agent.pikeAngle) };
    Vector2 weaponEnd = Vector2Add(pos, Vector2Scale(pikeDir, pikeLength));

    float speed = Vector2Length(agent.velocity);
    Color pikeColor = { 150, 230, 150, 180 };
    if (speed > 40.0f) pikeColor = { 255, 230, 100, 200 };

    DrawLineEx(pos, weaponEnd, 1.2f, pikeColor);
    DrawCircleV(weaponEnd, 2.0f, { 255, 255, 255, 225 });
}

// --- HLAVNÍ FUNKCE ---
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pike Square Simulator - Prototype Milestone 1");
    SetTargetFPS(60);

    PikeSquare square(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 10, 10, 26.0f);
    VisualMode visualMode = VisualMode::ENGRAVING;
    Color paperBase = { 235, 222, 200, 255 };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f;

        // --- VSTUPY OD HRÁČE ---
        if (IsKeyPressed(KEY_T)) {
            visualMode = (visualMode == VisualMode::ENGRAVING) ? VisualMode::VECTOR : VisualMode::ENGRAVING;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            square.pikeState = (square.pikeState == PikeState::PORTE) ? PikeState::CHARGE : PikeState::PORTE;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            square.targetPos = GetMousePosition();
            square.hasWaypoint = true;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            square.hasWaypoint = false;
        }

        // --- AKTUALIZACE SIMULACE ---
        square.Update(dt);

        // --- VYKRESLENÍ SCÉNY ---
        BeginDrawing();

        if (visualMode == VisualMode::ENGRAVING) {
            ClearBackground(paperBase);

            for (int i = 0; i < SCREEN_WIDTH; i += 40) {
                DrawLine(i, 0, i, SCREEN_HEIGHT, { 0, 0, 0, 12 });
            }
            for (int j = 0; j < SCREEN_HEIGHT; j += 40) {
                DrawLine(0, j, SCREEN_WIDTH, j, { 0, 0, 0, 12 });
            }

            if (square.hasWaypoint) {
                DrawCircleLines(square.targetPos.x, square.targetPos.y, 8.0f, { 160, 50, 40, 150 });
                DrawCircle(square.targetPos.x, square.targetPos.y, 3.0f, { 160, 50, 40, 200 });
                DrawLineDashed(square.GetFrontCenter(), square.targetPos, 4.0f, 2.0f, { 160, 50, 40, 100 });
            }

            for (const auto& soldier : square.soldiers) {
                DrawEngravingAgent(soldier, square.spacing, square.pikeState);
            }

        } else {
            ClearBackground({ 15, 17, 22, 255 });

            // Výpočet mřížky pro 100 pikenýrů (0 až 99)
            Vector2 topLeft = square.soldiers[0].targetSlot;
            Vector2 topRight = square.soldiers[square.cols - 1].targetSlot;
            Vector2 bottomLeft = square.soldiers[(square.rows - 1) * square.cols].targetSlot;
            Vector2 bottomRight = square.soldiers[square.rows * square.cols - 1].targetSlot;

            DrawLineV(topLeft, topRight, { 0, 255, 100, 60 });
            DrawLineV(topRight, bottomRight, { 0, 255, 100, 60 });
            DrawLineV(bottomRight, bottomLeft, { 0, 255, 100, 60 });
            DrawLineV(bottomLeft, topLeft, { 0, 255, 100, 60 });

            if (square.hasWaypoint) {
                DrawCircleLines(square.targetPos.x, square.targetPos.y, 12.0f, { 0, 255, 255, 200 });
                DrawLineV(square.GetFrontCenter(), square.targetPos, { 0, 255, 255, 100 });
            }

            for (const auto& soldier : square.soldiers) {
                DrawVectorAgent(soldier, square.spacing, square.pikeState);
            }
        }

        // --- UŽIVATELSKÉ ROZHRANÍ (UI PANEL S VYSOKÝM ROZLIŠENÍM) ---
        DrawRectangle(10, 10, 480, 320, { 25, 25, 30, 220 });
        DrawRectangleLines(10, 10, 480, 320, { 100, 100, 110, 255 });

        // Titulek
        DrawText("PIKENYRSKY CTVEREC - PROTOTYP V1.2", 20, 20, 20, RAYWHITE);
        DrawText("Fyzikalne sjednocene veleni", 20, 44, 13, GOLD);
        DrawLine(20, 62, 460, 62, { 100, 100, 110, 255 });

        // Stav formace
        std::string modeStr = (visualMode == VisualMode::ENGRAVING) ? "Rytina (Historicky)" : "Vektorovy (Takticky)";
        DrawText(TextFormat("Vizuální styl [T]: %s", modeStr.c_str()), 20, 75, 16, LIGHTGRAY);

        std::string pikesStr = (square.pikeState == PikeState::PORTE) ? "PORTE (Na rameno / Rychly)" : "CHARGE (K utoku / Pomaly)";
        Color stateColor = (square.pikeState == PikeState::PORTE) ? SKYBLUE : ORANGE;
        DrawText(TextFormat("Stav pík [MEZERNIK]: %s", pikesStr.c_str()), 20, 100, 16, stateColor);

        // Dynamické vyobrazení tempa formace (NCO kontrola)
        float tempoFactor = square.GetNcoTempoFactor();
        Color tempoColor = GREEN;
        if (tempoFactor < 0.90f) tempoColor = ORANGE;
        if (tempoFactor < 0.50f) tempoColor = RED;
        DrawText(TextFormat("Tempo formace (vliv NCO): %.0f %%", tempoFactor * 100.0f), 20, 125, 16, tempoColor);

        // Zobrazení indexu soudržnosti (Cohesion)
        float cohesion = square.GetCohesionIndex();
        Color cohesionColor = GREEN;
        if (cohesion < 0.85f) cohesionColor = YELLOW;
        if (cohesion < 0.65f) cohesionColor = RED;

        DrawText(TextFormat("Soudržnost čtverce: %.1f %%", cohesion * 100.0f), 20, 150, 16, RAYWHITE);
        DrawRectangle(20, 175, 440, 18, { 50, 50, 60, 255 });
        DrawRectangle(20, 175, (int)(440.0f * cohesion), 18, cohesionColor);

        // Ovládací prvky a nápověda (přizpůsobeno pro zobrazení Shift+A,D)
        DrawText("OVLÁDÁNÍ:", 20, 200, 14, GOLD);
        DrawText("- [W, S]: Pohyb vpřed/vzad", 20, 218, 13, LIGHTGRAY);
        DrawText("- [A, D]: Kolo (Rotace kolem př. LEVÉHO / PRAVÉHO rohu)", 20, 234, 13, LIGHTGRAY);
        DrawText("- [Shift + A, D]: Obrat na místě o 90° (Změna čela čtverce)", 20, 250, 13, LIGHTGRAY);
        DrawText("- [Klik myši L]: Waypoint (Pochod za středem 1. řady)", 20, 266, 13, LIGHTGRAY);
        DrawText("- [Klik myši R]: Zrušit waypoint", 20, 282, 13, LIGHTGRAY);
        DrawText("* NCO v rozích zpomalují rotaci, pokud nestíhají vnější poloměr!", 20, 301, 12, GOLD);

        // Vykreslení legendy do pravého rohu dole
        DrawRectangle(SCREEN_WIDTH - 250, SCREEN_HEIGHT - 150, 230, 130, { 25, 25, 30, 220 });
        DrawRectangleLines(SCREEN_WIDTH - 250, SCREEN_HEIGHT - 150, 230, 130, { 100, 100, 110, 255 });
        DrawText("LEGENDA JEDNOTEK:", SCREEN_WIDTH - 240, SCREEN_HEIGHT - 140, 13, GOLD);

        DrawCircle(SCREEN_WIDTH - 230, SCREEN_HEIGHT - 110, 7, { 180, 50, 40, 255 });
        DrawText("Velitel (Kapitán vzadu)", SCREEN_WIDTH - 212, SCREEN_HEIGHT - 117, 13, RAYWHITE);

        DrawCircle(SCREEN_WIDTH - 230, SCREEN_HEIGHT - 85, 7, { 190, 130, 40, 255 });
        DrawText("Poddůstojník (NCO na rozích)", SCREEN_WIDTH - 212, SCREEN_HEIGHT - 92, 13, RAYWHITE);

        DrawCircle(SCREEN_WIDTH - 230, SCREEN_HEIGHT - 60, 7, { 120, 110, 100, 255 });
        DrawText("Běžný pikenýr uvnitř", SCREEN_WIDTH - 212, SCREEN_HEIGHT - 67, 13, RAYWHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}