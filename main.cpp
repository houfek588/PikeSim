//
// Created by houfe on 17.06.2026.
//

#include "main.h"

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <string>

// --- KONSTANTY A NASTAVENÍ ---
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;

enum class Role {
    PIKEMAN,    // Řadový pikenýr
    NCO,        // Poddůstojník (v rozích)
    LEADER      // Kapitán / Velitel (vpředu uprostřed)
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

        // Inicializace 100 vojáků v mřížce
        int idCounter = 0;
        float colOffset = (cols - 1) / 2.0f;
        float rowOffset = (rows - 1) / 2.0f;

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                Agent agent;
                agent.id = idCounter++;

                // Určení role na základě pozice v mřížce
                if (row == 0 && col == cols / 2) {
                    agent.role = Role::LEADER;
                    agent.mass = 1.2f;
                    agent.maxForce = 350.0f; // Kapitán táhne nejsilněji
                } else if ((row == 0 && col == 0) ||
                           (row == 0 && col == cols - 1) ||
                           (row == rows - 1 && col == 0) ||
                           (row == rows - 1 && col == cols - 1)) {
                    agent.role = Role::NCO;
                    agent.mass = 1.1f;
                    agent.maxForce = 280.0f; // Poddůstojníci pevně drží rohy
                } else {
                    agent.role = Role::PIKEMAN;
                    agent.mass = 1.0f;
                    agent.maxForce = 180.0f; // Řadoví vojáci mají větší setrvačnost
                }

                // Výpočet počáteční pozice v lokálních souřadnicích mřížky
                float localX = (col - colOffset) * spacing;
                float localY = (row - rowOffset) * spacing;

                // Rotace lokální pozice do globálního světa
                Vector2 rotatedOffset = {
                    localX * cosf(angle) - localY * sinf(angle),
                    localX * sinf(angle) + localY * cosf(angle)
                };

                agent.position = Vector2Add(position, rotatedOffset);
                agent.velocity = { 0, 0 };
                agent.targetSlot = agent.position;
                agent.angle = angle;
                agent.pikeAngle = angle;
                agent.swingTimer = (float)GetRandomValue(0, 100) / 10.0f; // Náhodná fáze houpání

                soldiers.push_back(agent);
            }
        }
    }

    // Výpočet ideálních pozic mřížky na základě aktuálního středu a rotace
    void UpdateTargetSlots() {
        float colOffset = (cols - 1) / 2.0f;
        float rowOffset = (rows - 1) / 2.0f;
        int index = 0;

        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                float localX = (col - colOffset) * spacing;
                float localY = (row - rowOffset) * spacing;

                Vector2 rotatedOffset = {
                    localX * cosf(angle) - localY * sinf(angle),
                    localX * sinf(angle) + localY * cosf(angle)
                };

                soldiers[index].targetSlot = Vector2Add(position, rotatedOffset);
                index++;
            }
        }
    }

    void Update(float dt) {
        // 1. Řízení pohybu pivotu (klávesnice / waypoint)
        float speed = (pikeState == PikeState::PORTE) ? 80.0f : 45.0f; // Za pochodu s píkami dolů se jde pomaleji
        float rotationSpeed = 0.8f;
        Vector2 moveDir = { 0, 0 };

        if (IsKeyDown(KEY_W)) {
            moveDir.x += cosf(angle);
            moveDir.y += sinf(angle);
        }
        if (IsKeyDown(KEY_S)) {
            moveDir.x -= cosf(angle);
            moveDir.y -= sinf(angle);
        }
        if (IsKeyDown(KEY_A)) {
            angle -= rotationSpeed * dt;
        }
        if (IsKeyDown(KEY_D)) {
            angle += rotationSpeed * dt;
        }

        // Ovládání pomocí waypointu (kliknutí myší)
        if (hasWaypoint) {
            Vector2 toWaypoint = Vector2Subtract(targetPos, position);
            float dist = Vector2Length(toWaypoint);
            if (dist > 5.0f) {
                Vector2 dir = Vector2Normalize(toWaypoint);
                moveDir = Vector2Add(moveDir, dir);

                // Plynulé otáčení směrem k cíli
                float targetAngleNew = atan2f(dir.y, dir.x);
                // Vyhlazení přechodu úhlu (předcházení skokům o 360 stupňů)
                float angleDiff = targetAngleNew - angle;
                while (angleDiff < -PI) angleDiff += 2 * PI;
                while (angleDiff > PI) angleDiff -= 2 * PI;
                angle += angleDiff * 2.0f * dt;
            } else {
                hasWaypoint = false;
            }
        }

        if (Vector2Length(moveDir) > 0.01f) {
            moveDir = Vector2Normalize(moveDir);
            position = Vector2Add(position, Vector2Scale(moveDir, speed * dt));
        }

        // Udržování úhlu v rozmezí -PI až PI
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
                float k = (agent.role == Role::LEADER) ? 12.0f : (agent.role == Role::NCO ? 9.0f : 6.0f);
                springForce = Vector2Scale(Vector2Normalize(toSlot), distToSlot * k);
            }

            // Repulzní síla mezi vojáky (aby do sebe nenaráželi a drželi rozestup)
            Vector2 separationForce = { 0, 0 };
            float personalRadius = spacing * 0.75f;
            int neighborsCount = 0;

            for (const auto& other : soldiers) {
                if (agent.id == other.id) continue;
                Vector2 toAgent = Vector2Subtract(agent.position, other.position);
                float dist = Vector2Length(toAgent);
                if (dist < personalRadius && dist > 0.01f) {
                    float push = (personalRadius - dist) / personalRadius;
                    separationForce = Vector2Add(separationForce, Vector2Scale(Vector2Normalize(toAgent), push * 300.0f));
                    neighborsCount++;
                }
            }

            // Výsledná síla aplikovaná na agenta
            Vector2 totalForce = Vector2Add(springForce, separationForce);

            // Omezení maximální síly podle role (velitelé mají větší sílu se prosadit)
            float forceMag = Vector2Length(totalForce);
            if (forceMag > agent.maxForce) {
                totalForce = Vector2Scale(Vector2Normalize(totalForce), agent.maxForce);
            }

            // Eulerova integrace
            Vector2 acceleration = Vector2Scale(totalForce, 1.0f / agent.mass);
            agent.velocity = Vector2Add(agent.velocity, Vector2Scale(acceleration, dt));

            // Tlumení pohybu (odpor prostředí a tření nohou o zem)
            agent.velocity = Vector2Scale(agent.velocity, 0.85f);
            agent.position = Vector2Add(agent.position, Vector2Scale(agent.velocity, dt));

            // Otáčení vojáka ve směru formace
            float angleDiff = angle - agent.angle;
            while (angleDiff < -PI) angleDiff += 2 * PI;
            while (angleDiff > PI) angleDiff -= 2 * PI;
            agent.angle += angleDiff * 5.0f * dt;

            // 3. Fyzikální houpání (Swaying) píky za chůze
            float speedCurrent = Vector2Length(agent.velocity);
            if (speedCurrent > 5.0f) {
                agent.swingTimer += speedCurrent * 0.1f * dt;
            } else {
                // Pomalé uklidnění píky do stabilní polohy, pokud voják stojí
                agent.swingTimer = Lerp(agent.swingTimer, 0.0f, 5.0f * dt);
            }

            float swayAmplitude = (pikeState == PikeState::PORTE) ? 0.06f : 0.12f;
            float sway = sinf(agent.swingTimer) * swayAmplitude;

            // Nastavení úhlu píky podle stavu formace
            if (pikeState == PikeState::PORTE) {
                // Vertikální nesení píky (mírně nakloněná vpřed + houpání do stran)
                agent.pikeAngle = agent.angle - 0.1f + sway;
            } else {
                // Skloněná píka k útoku (přímo vpřed + vertikální kmitání)
                agent.pikeAngle = agent.angle + sway;
            }
        }
    }

    // Výpočet indexu soudržnosti (0.0f = totální rozpad, 1.0f = dokonalá formace)
    float GetCohesionIndex() const {
        float totalDeviation = 0.0f;
        for (const auto& agent : soldiers) {
            totalDeviation += Vector2Distance(agent.position, agent.targetSlot);
        }
        float avgDeviation = totalDeviation / soldiers.size();
        // Pokud je průměrná odchylka větší než spacing * 1.5, soudržnost klesá k nule
        float maxAllowedDev = spacing * 1.5f;
        float cohesion = 1.0f - (avgDeviation / maxAllowedDev);
        if (cohesion < 0.0f) cohesion = 0.0f;
        return cohesion;
    }
};

// --- FUNKCE PRO VYKRESLENÍ ---
void DrawEngravingAgent(const Agent& agent, float spacing, PikeState state) {
    // Barvy pro styl staré rytiny (odstíny inkoustu a sépie)
    Color inkDark = { 45, 38, 34, 255 };
    Color inkLight = { 120, 110, 100, 255 };
    Color plumeColor = inkLight;

    if (agent.role == Role::LEADER) plumeColor = { 180, 50, 40, 255 }; // Výrazné červené pero velitele
    else if (agent.role == Role::NCO) plumeColor = { 190, 130, 40, 255 }; // Oranžové/žluté pero poddůstojníka

    Vector2 pos = agent.position;
    float r = spacing * 0.35f;

    // Kreslení stínu na papíře
    DrawCircleSector(Vector2Add(pos, {2, 3}), r, 0, 360, 8, {0, 0, 0, 40});

    // Tělo vojáka (Základní kruh)
    DrawCircleSector(pos, r, 0, 360, 12, inkLight);
    DrawCircleLines(pos.x, pos.y, r, inkDark);

    // Směrová čára ramen (kolmo na pohled)
    Vector2 leftShoulder = { pos.x - r * sinf(agent.angle), pos.y + r * cosf(agent.angle) };
    Vector2 rightShoulder = { pos.x + r * sinf(agent.angle), pos.y - r * cosf(agent.angle) };
    DrawLineEx(leftShoulder, rightShoulder, 1.5f, inkDark);

    // Přilba (středový lesklý bod s inkoustovým okrajem)
    DrawCircle(pos.x, pos.y, r * 0.5f, { 220, 220, 220, 255 });
    DrawCircleLines(pos.x, pos.y, r * 0.5f, inkDark);

    // Plume (pero na přilbě) směrem dozadu
    Vector2 plumeDir = { -cosf(agent.angle), -sinf(agent.angle) };
    Vector2 plumePos = Vector2Add(pos, Vector2Scale(plumeDir, r * 0.6f));
    DrawCircle(plumePos.x, plumePos.y, r * 0.3f, plumeColor);
    DrawCircleLines(plumePos.x, plumePos.y, r * 0.3f, inkDark);

    // Vykreslení zbraně (Píka / Halapartna)
    float pikeLength = (state == PikeState::PORTE) ? spacing * 1.8f : spacing * 3.5f;
    Vector2 pikeDir = { cosf(agent.pikeAngle), sinf(agent.pikeAngle) };

    // Počátek zbraně (v pravé ruce/rameni)
    Vector2 weaponStart = Vector2Add(pos, Vector2Scale({ sinf(agent.angle), -cosf(agent.angle) }, r * 0.4f));
    Vector2 weaponEnd = Vector2Add(weaponStart, Vector2Scale(pikeDir, pikeLength));

    // Stín píky na papíře
    DrawLineEx(Vector2Add(weaponStart, {3, 5}), Vector2Add(weaponEnd, {3, 5}), 1.0f, {0, 0, 0, 30});

    if (agent.role == Role::NCO) {
        // Poddůstojník má halapartnu (kratší ratiště, široká sekera na konci)
        float halberdLength = spacing * 1.5f;
        Vector2 halberdEnd = Vector2Add(weaponStart, Vector2Scale(pikeDir, halberdLength));
        // Ratiště
        DrawLineEx(weaponStart, halberdEnd, 1.8f, { 95, 75, 55, 255 }); // hnědé dřevo
        DrawLineEx(weaponStart, halberdEnd, 1.0f, inkDark);
        // Hrot a sekera halapartny
        Vector2 headPos = Vector2Subtract(halberdEnd, Vector2Scale(pikeDir, 3.0f));
        DrawCircleSector(headPos, 4.0f, 0, 360, 4, { 180, 185, 190, 255 });
        DrawCircleSectorLines(headPos, 4.0f, 0, 360, 4, inkDark);
    } else if (agent.role == Role::LEADER) {
        // Velitel má tasený meč směřující dopředu
        float swordLength = spacing * 1.1f;
        Vector2 swordEnd = Vector2Add(weaponStart, Vector2Scale(pikeDir, swordLength));
        DrawLineEx(weaponStart, swordEnd, 1.5f, { 200, 200, 205, 255 });
        DrawLineEx(weaponStart, swordEnd, 0.8f, inkDark);
    } else {
        // Klasická dlouhá píka
        // Ratiště (dřevěná dlouhá tyč)
        DrawLineEx(weaponStart, weaponEnd, 1.5f, { 115, 85, 60, 255 }); // dřevěný tón
        DrawLineEx(weaponStart, weaponEnd, 0.8f, inkDark);

        // Kovový hrot píky
        Vector2 tipBack = Vector2Subtract(weaponEnd, Vector2Scale(pikeDir, 6.0f));
        Vector2 tipLeft = { tipBack.x - 2.5f * sinf(agent.pikeAngle), tipBack.y + 2.5f * cosf(agent.pikeAngle) };
        Vector2 tipRight = { tipBack.x + 2.5f * sinf(agent.pikeAngle), tipBack.y - 2.5f * cosf(agent.pikeAngle) };

        DrawTriangle(weaponEnd, tipLeft, tipRight, { 190, 195, 200, 255 });
        DrawTriangleLines(weaponEnd, tipLeft, tipRight, inkDark);
    }
}

void DrawVectorAgent(const Agent& agent, float spacing, PikeState state) {
    Color coreColor = { 0, 200, 255, 255 };      // Cyan pro běžné pikenýry
    if (agent.role == Role::LEADER) coreColor = { 255, 50, 50, 255 };  // Červená pro velitele
    else if (agent.role == Role::NCO) coreColor = { 255, 180, 0, 255 }; // Žlutá pro poddůstojníky

    Vector2 pos = agent.position;
    float r = spacing * 0.25f;

    // Vykreslení ideálního slotu (tenký terčík)
    DrawCircleLines(agent.targetSlot.x, agent.targetSlot.y, 3.0f, { 100, 100, 100, 100 });
    // Čára spojující vojáka s jeho slotem (vizualizace napětí pružiny)
    DrawLineV(pos, agent.targetSlot, { 255, 255, 255, 50 });

    // Tělo vojáka (Svítící bod)
    DrawCircleV(pos, r, coreColor);
    DrawCircleLines(pos.x, pos.y, r + 1, { 255, 255, 255, 150 });

    // Vektor pohybu (zelená čára ukazuje rychlost a směr)
    Vector2 velEnd = Vector2Add(pos, Vector2Scale(agent.velocity, 0.15f));
    DrawLineV(pos, velEnd, { 0, 255, 100, 180 });

    // Píka jako ostrá vektorová linka
    float pikeLength = (state == PikeState::PORTE) ? spacing * 1.5f : spacing * 3.2f;
    Vector2 pikeDir = { cosf(agent.pikeAngle), sinf(agent.pikeAngle) };
    Vector2 weaponEnd = Vector2Add(pos, Vector2Scale(pikeDir, pikeLength));

    // Barva píky reflektuje rychlost (stres / dynamiku pohybu)
    float speed = Vector2Length(agent.velocity);
    Color pikeColor = { 150, 230, 150, 180 };
    if (speed > 40.0f) pikeColor = { 255, 230, 100, 200 }; // Žlutá při prudším manévru

    DrawLineEx(pos, weaponEnd, 1.2f, pikeColor);
    DrawCircleV(weaponEnd, 2.0f, { 255, 255, 255, 225 }); // Svítící hrot
}

// --- HLAVNÍ FUNKCE ---
int main() {
    // Inicializace okna
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pike Square Simulator - Prototype Milestone 1");
    SetTargetFPS(60);

    // Inicializace pikenýrského čtverce uprostřed obrazovky
    PikeSquare square(SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 10, 10, 26.0f);

    VisualMode visualMode = VisualMode::ENGRAVING;

    // Načtení barev pro pozadí starého papíru
    Color paperBase = { 235, 222, 200, 255 };
    Color paperShadow = { 220, 206, 182, 255 };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f; // Prevence velkých skoků při záseku FPS

        // --- VSTUPY OD HRÁČE ---

        // Přepínání vizuálního módu (T)
        if (IsKeyPressed(KEY_T)) {
            visualMode = (visualMode == VisualMode::ENGRAVING) ? VisualMode::VECTOR : VisualMode::ENGRAVING;
        }

        // Přepínání stavu pík (Mezerník)
        if (IsKeyPressed(KEY_SPACE)) {
            square.pikeState = (square.pikeState == PikeState::PORTE) ? PikeState::CHARGE : PikeState::PORTE;
        }

        // Nastavení waypointu kliknutím pravým nebo levým tlačítkem myši
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            square.targetPos = GetMousePosition();
            square.hasWaypoint = true;
        }

        // Zrušení waypointu (pravým tlačítkem)
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            square.hasWaypoint = false;
        }

        // --- AKTUALIZACE SIMULACE ---
        square.Update(dt);

        // --- VYKRESLENÍ SCÉNY ---
        BeginDrawing();

        if (visualMode == VisualMode::ENGRAVING) {
            // Režim staré rytiny
            ClearBackground(paperBase);

            // Vykreslení jemné dobové mřížky/textury na papíře
            for (int i = 0; i < SCREEN_WIDTH; i += 40) {
                DrawLine(i, 0, i, SCREEN_HEIGHT, { 0, 0, 0, 12 });
            }
            for (int j = 0; j < SCREEN_HEIGHT; j += 40) {
                DrawLine(0, j, SCREEN_WIDTH, j, { 0, 0, 0, 12 });
            }

            // Vykreslení waypointu
            if (square.hasWaypoint) {
                DrawCircleLines(square.targetPos.x, square.targetPos.y, 8.0f, { 160, 50, 40, 150 });
                DrawCircle(square.targetPos.x, square.targetPos.y, 3.0f, { 160, 50, 40, 200 });
                DrawLineDashed(square.position, square.targetPos, 4.0f, 2.0f, { 160, 50, 40, 100 });
            }

            // Vykreslení všech vojáků
            for (const auto& soldier : square.soldiers) {
                DrawEngravingAgent(soldier, square.spacing, square.pikeState);
            }

        } else {
            // Minimalistický vektorový režim
            ClearBackground({ 15, 17, 22, 255 });

            // Vykreslení obrysu cílové mřížky formace
            Vector2 topLeft = square.soldiers[0].targetSlot;
            Vector2 topRight = square.soldiers[square.cols - 1].targetSlot;
            Vector2 bottomLeft = square.soldiers[square.soldiers.size() - square.cols].targetSlot;
            Vector2 bottomRight = square.soldiers[square.soldiers.size() - 1].targetSlot;

            DrawLineV(topLeft, topRight, { 0, 255, 100, 60 });
            DrawLineV(topRight, bottomRight, { 0, 255, 100, 60 });
            DrawLineV(bottomRight, bottomLeft, { 0, 255, 100, 60 });
            DrawLineV(bottomLeft, topLeft, { 0, 255, 100, 60 });

            // Vykreslení waypointu
            if (square.hasWaypoint) {
                DrawCircleLines(square.targetPos.x, square.targetPos.y, 12.0f, { 0, 255, 255, 200 });
                DrawLineV(square.position, square.targetPos, { 0, 255, 255, 100 });
            }

            // Vykreslení všech vojáků
            for (const auto& soldier : square.soldiers) {
                DrawVectorAgent(soldier, square.spacing, square.pikeState);
            }
        }

        // --- UŽIVATELSKÉ ROZHRANÍ (UI PANEL) ---

        // Polotransparentní podklad pro dashboard
        DrawRectangle(10, 10, 420, 240, { 25, 25, 30, 220 });
        DrawRectangleLines(10, 10, 420, 240, { 100, 100, 110, 255 });

        // Texty rozhraní
        DrawText("PIKENYRSKY CTVEREC - PROTOTYP V1.0", 20, 20, 16, RAYWHITE);
        DrawLine(20, 42, 400, 42, { 100, 100, 110, 255 });

        // Stav formace
        std::string modeStr = (visualMode == VisualMode::ENGRAVING) ? "Rytina (Historicky)" : "Vektorovy (Takticky)";
        DrawText(TextFormat("Vizuální styl [T]: %s", modeStr.c_str()), 20, 55, 14, LIGHTGRAY);

        std::string pikesStr = (square.pikeState == PikeState::PORTE) ? "PORTE (Na rameno / Rychly)" : "CHARGE (K utoku / Pomaly)";
        Color stateColor = (square.pikeState == PikeState::PORTE) ? SKYBLUE : ORANGE;
        DrawText(TextFormat("Stav pík [MEZERNIK]: %s", pikesStr.c_str()), 20, 80, 14, stateColor);

        // Zobrazení indexu soudržnosti (Cohesion)
        float cohesion = square.GetCohesionIndex();
        Color cohesionColor = GREEN;
        if (cohesion < 0.85f) cohesionColor = YELLOW;
        if (cohesion < 0.65f) cohesionColor = RED;

        DrawText(TextFormat("Soudržnost čtverce: %.1f %%", cohesion * 100.0f), 20, 110, 14, RAYWHITE);
        DrawRectangle(20, 130, 380, 15, { 50, 50, 60, 255 });
        DrawRectangle(20, 130, (int)(380.0f * cohesion), 15, cohesionColor);

        // Ovládací prvky a nápověda
        DrawText("OVLÁDÁNÍ:", 20, 155, 12, GOLD);
        DrawText("- [W, S]: Pohyb vpřed/vzad", 20, 175, 12, LIGHTGRAY);
        DrawText("- [A, D]: Kolo (Rotace formace)", 20, 190, 12, LIGHTGRAY);
        DrawText("- [Klik myši L]: Nastavit pochodový waypoint", 20, 205, 12, LIGHTGRAY);
        DrawText("- [Klik myši R]: Zrušit waypoint", 20, 220, 12, LIGHTGRAY);

        // Vykreslení malého legendy do pravého rohu dole
        DrawRectangle(SCREEN_WIDTH - 220, SCREEN_HEIGHT - 130, 200, 110, { 25, 25, 30, 220 });
        DrawRectangleLines(SCREEN_WIDTH - 220, SCREEN_HEIGHT - 130, 200, 110, { 100, 100, 110, 255 });
        DrawText("LEGENDA JEDNOTEK:", SCREEN_WIDTH - 210, SCREEN_HEIGHT - 120, 11, GOLD);

        DrawCircle(SCREEN_WIDTH - 200, SCREEN_HEIGHT - 95, 6, { 180, 50, 40, 255 });
        DrawText("Velitel (Kapitán)", SCREEN_WIDTH - 185, SCREEN_HEIGHT - 100, 11, RAYWHITE);

        DrawCircle(SCREEN_WIDTH - 200, SCREEN_HEIGHT - 75, 6, { 190, 130, 40, 255 });
        DrawText("Poddůstojník (NCO)", SCREEN_WIDTH - 185, SCREEN_HEIGHT - 80, 11, RAYWHITE);

        DrawCircle(SCREEN_WIDTH - 200, SCREEN_HEIGHT - 55, 6, { 120, 110, 100, 255 });
        DrawText("Běžný pikenýr", SCREEN_WIDTH - 185, SCREEN_HEIGHT - 60, 11, RAYWHITE);

        EndDrawing();
    }

    // Vyčištění a zavření okna
    CloseWindow();

    return 0;
}