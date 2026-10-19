# PikeSim: Herní Idea a Rozcestník Dokumentace

Vítejte v projektu **PikeSim** (*PikeSquareSimulator*) – autentickém fyzikálním simulátoru renesančních formací pikenýrů, mušketýrských salv a brutální mechaniky **„Push of Pikes“**.

---

## 📚 Oficiální Dokumentace Projektu

Původní koncepty a brainstorming byly detailně formalizovány a rozděleny do specializovaných dokumentů:

1. **[Game Design Document (docs/game_design.md)](file:///C:/Projects/RT_TrMan/PikeSimulator/PikeSim/docs/game_design.md)**  
   *Kompletní designová bible hry:*
   * **Role hráče a herní režimy:** Taktický sandbox (laboratoř) a historické výzvy/scénáře.
   * **Mikro-fyzika pík:** Pružné úsečky, torzní odpor, lámání pík a 4 fáze střetu (*Přiblížení*, *Propletení*, *Push*, *Bad War*).
   * **Složení jednotek a zbroj:** Pikenýři v kyrysech, halapartníci/Doppelsöldneři, mušketýři s kontramarší.
   * **Mechanika padlých:** Těla jako fyzické překážky a automatické doplňování mezer ze zadních řad.
   * **Akustické velení a kurýři:** Šíření rozkazů zvukem bubnů, útlum kouřem a hlukem, fyzické entity poslů.
   * **Simulace kouře a Fog of War:** Částicové pole kouře, vliv větru, akustický filtr a taktický rentgen.
   * **Matematické vzorce:** Pružiny, odpuzování, kumulace tlaku a index soudržnosti (*Cohesion*).

2. **[Systémová Architektura (docs/architecture.md)](file:///C:/Projects/RT_TrMan/PikeSimulator/PikeSim/docs/architecture.md)**  
   *Technická specifikace enginu a kódu:*
   * Dekompozice do modulů: `core/`, `simulation/`, `formation/`, `rendering/`, `ui/`.
   * Diagramy subsystémů a životního cyklu snímku (Mermaid).
   * Prostorové indexování ([`SpatialHashGrid`](file:///C:/Projects/RT_TrMan/PikeSimulator/PikeSim/docs/architecture.md#L42)) pro škálování kolizí $O(N)$.
   * Cílová adresářová struktura a roadmapa fází 1–5.

3. **[Průvodce pro Agenty a Přispěvatele (AGENTS.md)](file:///C:/Projects/RT_TrMan/PikeSimulator/PikeSim/AGENTS.md)**  
   *Pravidla pro vývoj a práci v repozitáři:*
   * Návod na spouštění přes [run.bat](file:///C:/Projects/RT_TrMan/PikeSimulator/PikeSim/run.bat) (využívá vestavěný MinGW z CLionu).
   * Výkonnostní invarianty (žádné alokace v ticku, stabilita matematiky).
   * Konvence C++17 a Raylibu.

---

## ⚔️ Rychlé shrnutí základní vize

* **Proč ne klasická RTS?** Tradiční strategie (Total War) reprezentují jednotky jako zjednodušené bloky životů s animacemi. V PikeSim je jednotka složena ze stovek samostatně simulovaných vojáků s fyzikálními vazbami a psychologickým tlakem.
* **Push of Pikes:** Když se srazí dva čtverce, nejde jen o to, kdo má větší útok. Rozhoduje hloubka formace, kinetická energie zadních řad, které fyzicky tlačí do předních, ohýbání a křížení stovek dříků pík a udržení rovných řad pod dohledem poddůstojníků (NCOs).
* **Vizuální zážitek:** Možnost přepínat stiskem klávesy `T` mezi imerzivním stylem staré papírové rytiny a analytickým laboratorním vektorem.
