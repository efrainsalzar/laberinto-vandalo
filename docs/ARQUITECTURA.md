# Arquitectura

> Cómo está organizado el código y por qué. Complementa [VISION.md §15](VISION.md#15-arquitectura-y-patrones-de-diseño).
> Índice corto: principios, mapa de módulos y patrones. El detalle de cada módulo está en `docs/arquitectura/`.

## 1. Principios

1. **Dos capas.**
   - **`core`** (C++ puro, namespace `lv::core`): reglas y algoritmos. No incluye nada de Godot.
     Se prueba con doctest en milisegundos, sin abrir el motor.
   - **`game`** (C++ con godot-cpp): nodos y recursos que Godot ve. Son **adaptadores**:
     traducen el mundo de Godot (posiciones, input, señales) a `core` y viceversa.
   - **GDScript** (`project/scripts`): solo interfaz (HUD, menús) y conectar escenas.
2. **El participante no sabe quién lo controla** (Strategy). Humano, bot o red producen las mismas órdenes.
3. **Composición sobre herencia.** Un participante *tiene* stats, visión, inventario; no *es* una subclase de cada cosa.
4. **Eventos en lugar de dependencias directas** (Observer / señales). Quien toma el trofeo no llama al HUD:
   emite `trophy_taken` y quien quiera escucha.
5. **Datos fuera del código** cuando varían por diseño (reglas, objetos): `Resource` de Godot o archivos.
6. **Un patrón solo si resuelve un problema actual.** Nada de capas "por si acaso".
7. **Event Bus** (`Events`, autoload): los niveles publican ahí lo que ocurrió; la interfaz, los sonidos y el
   Director escuchan ahí. La lógica C++ no conoce la interfaz. Los nodos C++ no dibujan; la vista no decide.
8. **Interfaz:** el centro de la pantalla queda libre; HUD en los bordes; ayuda en F1; textos flotantes para
   feedback. Cada opción de menú lleva descripción con `UiFx.hint(control, "...")` (globo flotante, no texto fijo).
9. **Rendimiento y escalabilidad:** ver [RENDIMIENTO.md](RENDIMIENTO.md) (presupuesto, `PerfScope`, panel F3,
   checklist de escalabilidad). Todo cambio del mapa va por `Maze.queue_passage_change`.

```
┌──────────────────────── Godot ─────────────────────────┐
│  Escenas (.tscn) + GDScript (HUD, menús)               │
│        │ señales ▲                                     │
│        ▼         │                                     │
│  game/  (nodos C++: Maze, Participant, MatchManager…)  │
│        │ llama   ▲ eventos / resultados                │
└────────┼─────────┼─────────────────────────────────────┘
         ▼         │
   core/  (C++ puro: MazeGrid, MatchState, Stats…)   ◄── tests/ (doctest)
```

## 2. Mapa de módulos (fase 1 — prototipo)

| # | Módulo | `core` (lógica) | `game` (Godot) | Patrón |
|---|---|---|---|---|
| 1 | **Laberinto** | `MazeGrid`: cuadrícula pared/suelo, generación con semilla, bóveda central, bases | `Maze` (`TileMapLayer`): dibuja la cuadrícula y da colisiones | Builder (generación por pasos) |
| 2 | **Participante** | — | `Participant` (`CharacterBody2D`): se mueve según su controlador | Composición |
| 3 | **Controladores** | `BotBrain` (Utility AI), `find_path` (A*) | `ParticipantController` → `HumanController`, `WanderController`, `BotController`; `BotCoordinator`, `BotProfile` | **Strategy**, Utility AI |
| 4 | **Stats** | `Stats`: valores base + modificadores | expuesto como recurso del participante | **Decorator / modificadores** |
| 5 | **Visión** | `line_of_sight`: radio + línea de visión | `Vision` (`PointLight2D` con sombras) | Composición, Observer |
| 6 | **Objetos** | `place_pickups`, `pick_spawn_cell`, `ExpiryQueue`, `Rng` | `PickupType` y `PickupCategory` (datos), `Pickup`, `PickupFactory`, `PickupSpawner` | **Factory + datos**, Strategy, Observer |
| 7 | **Trofeo y bases** | `TrophyTracker` | `TrophyController` + vistas | **State**, Observer, Factory |
| 8 | **Partida** | `MatchFlow`: reloj, fases, victoria/tiempo | `MatchController` + `MatchRules` (datos) | **State** (fases), Observer |

Fases siguientes (bots, combate, vándalos, director) se diseñan cuando lleguemos; ver [ROADMAP](ROADMAP.md).

## 3. Flujo de un frame (cuando la fase 1 esté completa)

1. Cada `Participant` pregunta a su **controlador** hacia dónde moverse y se mueve (física de Godot).
2. `MatchManager` convierte la posición de cada participante a casilla y se la pasa a `MatchState`.
3. `MatchState` aplica las reglas (¿recogió un punto? ¿tomó el trofeo? ¿llegó a su base?) y devuelve **eventos**.
4. `MatchManager` traduce los eventos a **señales** de Godot.
5. La interfaz (GDScript) y otros sistemas escuchan las señales.

Así las reglas viven en un solo lugar testeable, y Godot solo se encarga de mover, dibujar y escuchar.

---

## Detalle por módulo

Cada módulo tiene su archivo en [`arquitectura/`](arquitectura/). Los comentarios del código citan `ARQUITECTURA.md §N`: búscalo en esta tabla.
**Leer solo la sección que toca**, no todas.

| § | Módulo |
|---|---|
| §4 | [Módulo 1 — Laberinto (detalle)](arquitectura/04-laberinto.md) |
| §5 | [Módulo 2 — Participante (detalle)](arquitectura/05-participante.md) |
| §6 | [Módulo 3 — Stats (detalle)](arquitectura/06-stats.md) |
| §7 | [Módulo 4 — Visión limitada (detalle)](arquitectura/07-vision.md) |
| §8 | [Módulo 5 — Objetos recolectables (detalle)](arquitectura/08-objetos.md) |
| §9 | [Bloque UX-1 — Objetos infinitos (reaparición)](arquitectura/09-objetos-infinitos.md) |
| §10 | [Bloque UX-2 — HUD limpio y textos flotantes](arquitectura/10-hud.md) |
| §11 | [Etapa 1.1 — Trofeo y bases](arquitectura/11-trofeo-bases.md) |
| §12 | [Etapa 2.1 (parte) — Algoritmos de generación y dificultad](arquitectura/12-generacion-dificultad.md) |
| §13 | [Etapa 1.3 — Menús, flujo del juego y opciones](arquitectura/13-menus-flujo.md) |
| §14 | [Interfaz: efectos globales, descripciones flotantes y mapas grandes](arquitectura/14-interfaz-mapas-grandes.md) |
| §15 | [Etapa 2.4 (adelantada) — Laberinto vivo: mapas que cambian en tiempo real](arquitectura/15-laberinto-vivo.md) |
| §16 | [Rendimiento: medición y cola de cambios del mapa](arquitectura/16-rendimiento.md) |
| §17 | [Escalabilidad: prioridades y reservas, índice de entidades y prueba de estrés](arquitectura/17-escalabilidad.md) |
| §18 | [Etapa 1.2 — Partida: reloj, fases y fin por tiempo](arquitectura/18-partida-fases.md) |
| §19 | [Etapa 3 — Bots con dificultad](arquitectura/19-bots.md) |
| §20 | [Economía de objetos: categorías, caducidad y topes](arquitectura/20-economia-objetos.md) |
| §21 | [Lenguaje visual de objetos](arquitectura/21-lenguaje-visual.md) |

## Patrones aplicados (índice)

| Patrón | Dónde | Estado |
|---|---|---|
| Builder (pasos de generación) | `MazeGrid::generate` | ✅ |
| Strategy (algoritmos) | `carve_maze` + `MazeAlgorithm`; `MazeProfile` | ✅ |
| Strategy | `ParticipantController` (Human, Wander, Bot); `PickupSpawnPolicy`; `BotAction` | ✅ |
| Utility AI (puntajes) | `BotBrain` | ✅ |
| Fichas de ataque | `TokenPool` en `BotCoordinator` | ✅ |
| A* (búsqueda de caminos) | `find_path` | ✅ |
| Command | `ParticipantCommand` | ✅ |
| Composición / separación lógica-vista | `Participant` + `View` + `StatsComponent` + `Vision`; `core` vs `game`; `line_of_sight` vs luz | ✅ |
| Decorator / modificadores | `Stats` + `StatModifier` | ✅ |
| Observer / señales | `StatsComponent` → `HUD`, `Vision`; `Pickup` → `PickupFactory` → `Events` → UI; `pickup_spawned`; `MatchController` → `Events.match_*` → HUD | ✅ |
| State | `TrophyTracker` (estados del trofeo); `App` (flujo del juego); `MazeShifter` (laberinto vivo); `MatchFlow` (fases de la partida); luego estados del participante | ✅ (parcial) |
| Datos fuera del código | `StatsProfile`, `MatchRules`, `BotProfile`, `TrophyRules`, `PickupCategory` (.tres) | ✅ |
| Cola de prioridad (caducidad) | `ExpiryQueue` en `PickupSpawner` | ✅ |
| Factory + datos | `PickupFactory` + `PickupType` (.tres); vistas de trofeo/bases en `TrophyController` | ✅ |
| Event Bus | `Events` (autoload) | ✅ |
| Command (cola con presupuesto, prioridades y reservas) | `MapChangeQueue` en `Maze` | ✅ |
| Índice espacial | `SpatialHash` / `EntityIndex`; índice de objetos en `PickupFactory` | ✅ |
| Instrumentación / observabilidad | `Perf`, `PerfScope`, panel F3, monitores | ✅ |
| Object Pool | `FloatingTextLayer` | ✅ |
| Decorator en tiempo de ejecución | `UiFx` (efectos y descripciones en todos los controles) | ✅ |
