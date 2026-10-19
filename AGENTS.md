# AGENTS.md

Tento dokument slouží jako průvodce pro autonomní vývojové agenty (AI coding assistants) a přispěvatele pracující v repozitáři **PikeSim**.

---

## 1. O projektu PikeSim
**PikeSim** (*PikeSquareSimulator*) je fyzikální simulátor renesančních formací pikenýrů (španělské tercie, švýcarské čtverce) zaměřený na autentickou mikro-mechaniku střetu **„Push of Pikes“**.

* **Jazyk & Standard:** C++17
* **Grafika a okno:** [Raylib 5.0](https://www.raylib.com/) (získáván automaticky přes CMake `FetchContent`)
* **Build systém:** CMake (verze 3.20+) + Ninja

---

## 2. Příkazy pro sestavení a spuštění

Při práci na repozitáři **vždy ověřte sestavitelnost kódu** po každé úpravě.

### Rychlé spuštění z příkazové řádky (Doporučeno pro agenty i uživatele):
V kořenu projektu spusťte dávkový soubor [run.bat](file:///C:/Projects/RT_TrMan/PikeSimulator/PikeSim/run.bat):
```cmd
.\run.bat
```
Tento skript:
* Automaticky najde desktopový MinGW toolchain a CMake z instalace CLionu.
* Zkonfiguruje CMake projekt (pokud ještě neexistuje `cmake-build-cli`).
* Provede inkrementální build pomocí Ninja.
* Spustí aplikaci.

### Ruční sestavení přes CMake & Ninja:
```powershell
$env:PATH = "C:\Program Files\JetBrains\CLion 2025.3.3\bin\mingw\bin;C:\Program Files\JetBrains\CLion 2025.3.3\bin\ninja\win\x64;" + $env:PATH
cmake -B cmake-build-cli -G Ninja
ninja -C cmake-build-cli
```

---

## 3. Klíčová dokumentace repozitáře
Před většími úpravami nebo návrhem nových mechanik si prostudujte:
* [docs/game_design.md](file:///C:/Projects/RT_TrMan/PikeSimulator/PikeSim/docs/game_design.md) – Kompletní herní design dokument (GDD), fyzika pík, Push of Pikes, akustické velení, kouř.
* [docs/architecture.md](file:///C:/Projects/RT_TrMan/PikeSimulator/PikeSim/docs/architecture.md) – Systémová architektura, diagramy, datové struktury a roadmapa.
* [idea.md](file:///C:/Projects/RT_TrMan/PikeSimulator/PikeSim/idea.md) – Přehledový rozcestník vize projektu.

---

## 4. Architektonické a výkonnostní invarianty

Při psaní kódu pro PikeSim striktně dodržujte následující pravidla:

1. **Žádné dynamické alokace v herním ticku:**
   * Neprovádějte `new`, `delete`, ani opakované `std::vector::push_back` uvnitř `Update()` smyčky.
   * Agenti a kolizní entity musí být alokovány předem v souvislých polích (`std::vector<Agent>`).
2. **Škálovatelnost kolizí ($O(N)$ místo $O(N^2)$):**
   * Při rozšiřování fyziky nad rámec jednoho čtverce používejte prostorový index ([`SpatialHashGrid`](file:///C:/Projects/RT_TrMan/PikeSimulator/PikeSim/docs/architecture.md#L42)).
   * Vyvarujte se hrubých vnořených $N \times N$ cyklů pro kolize stovek agentů.
3. **Oddělení fyziky a vykreslování:**
   * Fyzika a logika pohybu (`PikeSquare`, `Agent`, `Flocking`) nesmí volat vykreslovací funkce Raylibu (`Draw*`).
   * Vykreslování je výhradně v kompetenci rendererů (`EngravingRenderer`, `VectorRenderer`).
4. **Matematická stabilita:**
   * Úhly vždy normalizujte do intervalu $[-\pi, \pi]$ (zabraňuje rotacím přes násobky $2\pi$).
   * Při normalizaci vektorů (`Vector2Normalize`) vždy ověřte, že délka vektoru není nulová (ochrana proti dělení nulou).
   * Omezujte maximální povolenou rychlost agentů rozpočtem `maxAllowedSpeed`.

---

## 5. Konvence a kódovací styl

* **Standard:** C++17.
* **Názvy tříd a struktur:** `PascalCase` (např. `PikeSquare`, `SpatialHashGrid`, `Agent`).
* **Názvy metod a funkcí:** `PascalCase` nebo `camelCase` konzistentně s okolním kódem.
* **Názvy členských proměnných:** `camelCase` (např. `targetSlot`, `pikeAngle`).
* **Konstanty a Enumy:** `UPPER_CASE` nebo `PascalCase` u `enum class` (např. `Role::PIKEMAN`, `PikeState::CHARGE`).
* **Komentáře:** Preferujte češtinu nebo angličtinu podle kontextu stávajícího souboru. V `main.cpp` je použita čeština.

---

## 6. Doménové koncepty simulace

* **Agent (`Agent`):** Jednotlivec s hmotností, rychlostí a cílovým slotem.
* **Role (`Role`):**
  * `PIKEMAN` – řadový voják v mřížce.
  * `NCO` – poddůstojník na rohu; jeho odchylka zpomaluje tempo celého čtverce (`GetNcoTempoFactor`).
  * `LEADER` – kapitán velící zezadu.
* **PikeState:**
  * `PORTE` – piky míří vzhůru, rychlý pochod, kymácení za chůze.
  * `CHARGE` – piky skloněny vodorovně, pomalý bojový pochod.
* **Manévry:**
  * Kolo (`A`/`D`): rotace kolem levého/pravého předního rohu formace.
  * Obrat na místě (`Shift + A/D`): permutace indexů v matici bez nutnosti obíhání formace.
