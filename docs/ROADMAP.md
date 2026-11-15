# Hoja de ruta y estado

La hoja de ruta completa del juego está en [VISION.md §17](VISION.md#17-hoja-de-ruta).
Aquí se lleva el **estado real** y el **siguiente paso**.

## Hecho

### v0.0.1 — Base del proyecto ✅
- [x] Entorno de desarrollo (Godot 4.7, godot-cpp 10, SCons, MSVC, VS Code).
- [x] Proyecto creado desde godot-cpp-template; la extensión se llama `laberinto`.
- [x] Tests unitarios con doctest (`scons tests=yes`).
- [x] Documentación: README, CLAUDE.md, VISION, CONTRIBUTING, CREDITS, LICENSE.
- [x] CI en GitHub Actions (tests + build Linux/Windows).

### Diseño de arquitectura ✅
- [x] [`ARQUITECTURA.md`](ARQUITECTURA.md): capas `core`/`game`, mapa de módulos de la fase 1 y patrones.

### v0.0.2 — Módulo 1: Laberinto ✅
- [x] `lv::core::MazeGrid`: generación por semilla (backtracking + bucles + bóveda central), bases, conectividad.
- [x] 9 tests unitarios.
- [x] Nodo `Maze` (`TileMapLayer`) con TileSet provisional de colores y colisión en las paredes.
- [x] Escena de prueba `scenes/sandbox/maze_sandbox.tscn` (R = otro laberinto).

### v0.0.3 — Laberinto grande + Módulo 2: Participante ✅
- [x] Pasillos anchos (3 casillas) con paredes de 1; laberinto de 20×14 celdas (81×57 casillas). +1 test.
- [x] `Participant` (`CharacterBody2D`) con controlador intercambiable (**Strategy**) que produce órdenes (**Command**).
- [x] `HumanController`: WASD y flechas.
- [x] Vista provisional separada de la lógica; escena de prueba con cámara que sigue al participante (Tab = mapa completo).

### v0.0.4 — Pieza 3: Stats ✅
- [x] `lv::core::Stats`: valor base + pila de modificadores (suma/multiplica, temporales/permanentes, por origen, límites). 6 tests.
- [x] `StatsComponent` (hijo del participante) + `StatsProfile` (datos .tres). El participante toma su velocidad de ahí.
- [x] Panel de stats arriba a la izquierda (Observer de las señales). Teclas 1/2/3/0 para probar efectos.

### v0.0.5 — Pieza 4: Visión limitada ✅
- [x] `lv::core::can_see` / `has_line_of_sight` (radio + línea de visión por casillas). 6 tests.
- [x] `Vision` (`PointLight2D` con sombras) como hijo del participante; el radio sigue a la stat `vision`.
- [x] Paredes con oclusores en su propia capa: se ven las cercanas, lo de detrás queda oculto.
- [x] Sandbox con oscuridad (tecla V la apaga).

### v0.0.6 — Pieza 5: Objetos recolectables ✅
- [x] Tipos de objeto como datos (`PickupType` + `PickupEffect`, `.tres`) y `PickupFactory` (Factory).
- [x] `place_pickups` (core): suelo, fuera de la bóveda, lejos de bases, separación y rareza. 5 tests.
- [x] `lv::core::Rng`: aleatorio portable (misma semilla = mismo resultado en todas las plataformas); `MazeGrid` migrado. 3 tests.
- [x] 3 tipos: punto de velocidad, punto de visión, power-up de rapidez (raro). Contador en el panel.

### v0.0.7 — Bloque UX-1: Objetos infinitos ✅
- [x] `PickupSpawner` + `PickupSpawnPolicy` (Strategy; hoy `KeepDensityPolicy`: 30 objetos, uno cada 3 s).
- [x] `pick_spawn_cell` (core): nunca aparece a la vista de un participante. 7 tests.
- [x] Animación de aparición.

### v0.0.8 — Bloque UX-2: HUD limpio y textos flotantes ✅
- [x] Event Bus `Events` (autoload); los niveles publican, la interfaz escucha.
- [x] HUD: barra superior delgada (stats que cuentan y destellan, fichas de efectos temporales con barra, contador); centro libre.
- [x] Textos flotantes con Object Pool (`+8 VEL`, `+0.5 VIS`, `¡RAPIDEZ! ×1.5 VEL`), visibles en la oscuridad.
- [x] Ayuda de controles en F1 (pista que se desvanece).

### v0.0.9 — Etapa 1.1: Trofeo y bases ✅
- [x] `TrophyTracker` (core, State): en bóveda → llevado → en el piso → entregado. 9 tests.
- [x] `TrophyController`: lentitud ×0.6 al portador, eventos al Event Bus, vistas de trofeo (con luz) y bases.
- [x] Flecha de objetivo en el borde (trofeo / tu base) con distancia; aviso "LLEVAS EL TROFEO"; cartel de victoria.

### v0.0.10 — Algoritmos de generación y dificultad ✅ (adelanto de la etapa 2.1)
- [x] 6 algoritmos clásicos (Backtracking, Hunt-and-Kill, Growing Tree, Kruskal, Wilson, Prim) sobre `PassageGrid` (topología).
- [x] Bucles y *braid* (eliminar callejones); métricas: % de callejones, cruces, ruta base → centro.
- [x] 4 niveles (Fácil, Normal, Difícil, Extremo) + personalizado como datos (`MazeProfile`). Tecla M en el sandbox.
- [x] 6 tests (52 en total), incluido uno que verifica que los niveles están ordenados por dificultad.

### v0.0.11 — Etapa 1.3: Menús, flujo del juego y opciones ✅
- [x] Menú principal (fondo de laberinto animado), configurar partida con **vista previa del mapa en vivo**, pausa, resultados, opciones persistentes.
- [x] Flujo del juego con **State** (`App`), opciones (`Settings`), `MatchConfig`, tema único (`ui/theme.tres`).
- [x] Todo con mouse, teclado y mando; teclas de prueba solo si se activan en Opciones.
- [x] La escena de prueba pasó a ser la partida real (`scenes/game/match.tscn`).

### v0.0.12 — Interfaz animada, mapas grandes y laberinto vivo ✅ (en prueba en vivo)
- [x] `UiFx`: hover = selección, rebote, barra de acento y descripciones flotantes en todos los menús.
- [x] Tamaños Normal/Grande/Enorme/Gigante; Personalizado hasta 160×110. Optimización: objetos dibujados una vez.
- [x] Laberinto vivo: cambios seguros (conectividad garantizada), aviso previo, espera/cancelación si hay alguien encima, objetos y trofeo se adaptan. 7 tests.

### v0.0.13 — Rendimiento: medición y escalabilidad ✅ (pendiente medición en vivo)
- [x] Panel de rendimiento F3 + monitores del depurador; `PerfScope` en todos los sistemas en caliente.
- [x] Cola de cambios del mapa (Command + presupuesto por frame); conectividad local acotada; caché de protegidos; muestreo; índice espacial de objetos.
- [x] `docs/RENDIMIENTO.md`: presupuesto, cómo medir, reglas. 62 tests.

### v0.0.14 — Escalabilidad: prioridades, índice espacial y prueba de estrés ✅
- [x] `MapChangeQueue`: prioridades (seguridad > jugador > Director > automático), reservas, tope 64, validación al aplicar.
- [x] `SpatialHash` + `EntityIndex`; `WanderController`.
- [x] Prueba de estrés en el menú principal con informe y veredicto. 7 tests (69 en total).

### v0.0.15 — Prueba de estrés automática con registro ✅
- [x] Informe JSON + historial CSV; captura de cada tirón con el costo de cada sistema en ese frame (`PerfStats.take_frame`).
- [x] Ejecución automática (`-- --stress --quit-after-stress`) y `verificar.ps1 -Estres`.
- [x] Dirty flag en participantes: prueba de estrés 5,2 → **1,2 ms** por frame (p99 ≈ 2,3 ms). DENTRO del presupuesto.

### v0.0.16 — Etapa 1.2: Partida (reloj y fases) + bases ajustadas al pasillo ✅
- [x] `MatchFlow` (core, State): Preparación → Asalto → Golpe de los vándalos → fin; victoria o tiempo agotado. 10 tests.
- [x] `MatchController` + `MatchRules` (.tres): 40/35/25, avisos a 60 y 10 s, laberinto vivo ×2 en el Golpe.
- [x] Reloj en el HUD, avisos de fase y tiempo, resultado "TIEMPO AGOTADO". Duración en el menú: 5/8/12/20 min, sin límite o personalizada (1–60).
- [x] Arreglo: la base mide el pasillo completo (antes 3×3 fijo, se metía en las paredes); el cuerpo del participante cabe en pasillos de 1. 2 tests (82 en total).
- [x] Tecla de prueba N: saltar a la siguiente fase.

### v0.0.18 — Quitar el trofeo (robo básico) y velocidad inicial pareja ✅
- [x] Robo por contacto: tocar al portador le quita el trofeo; inmunidad de 3 s al tomarlo o robarlo. Evento `trophy_stolen`.
- [x] Bots: nueva acción "quitar el trofeo" (persiguen al portador). Flecha "QUÍTASELO" y avisos en el HUD.
- [x] Todos empiezan con la misma velocidad, más lenta: 120 (antes 160); los objetos de velocidad pesan más.
- [x] 4 tests (99 en total). Reglas finas del robo (golpe, escudos anti-robo) llegan con el combate.

### v0.0.17 — Etapa 3: Bots con dificultad ✅
- [x] A* sobre celdas lógicas (con costos, listo para terrenos) y errores en cruces según la dificultad.
- [x] `BotBrain` (Utility AI): entregar, ir por el trofeo, recoger objetos VISTOS, explorar. Cada acción es una clase: agregar mecánicas = agregar acciones.
- [x] `BotProfile` .tres: Fácil, Normal, Difícil, Experto + "Mixto". `BotCoordinator`: cola de caminos (2/frame) y replanificación cuando el mapa cierra un camino.
- [x] Menú: rivales 0–7 y su dificultad. Resultado "DERROTA — ganó Bot X". Participantes se atraviesan (capas de física).
- [x] 13 tests (95 en total).

## Plan acordado (etapas, una pieza a la vez)

Bloques de interfaz ya hechos: ~~objetos infinitos~~ ✅ · ~~HUD limpio + textos flotantes~~ ✅.
Colores, arte e identidad visual se dejan para la etapa 7 (decisión del dueño del proyecto).
Ideas del mapa en detalle: [`DISENO_MAPA.md`](DISENO_MAPA.md).

### Etapa 1 — Jugable de principio a fin
1. ~~Bases + trofeo + ganar~~ ✅
2. ~~Partida: reloj, fases, victoria y fin por tiempo~~ ✅ (v0.0.16)
3. ~~Pantallas: inicio, configurar, pausa, resultados, opciones~~ ✅

~~Mapas que cambian en tiempo real~~ ✅ (v0.0.12, en prueba en vivo).

**Etapa 1 completa. Etapa 3 (bots) adelantada ✅.** Siguiente paso (orden propuesto por el dueño del proyecto):
acciones del participante → combate y robo del trofeo (Etapa 4) → objetos que cambian el mapa, paredes rompibles y
portales (Etapa 2.2–2.4) → formas de mapa (Etapa 2.1). Cada una entra como **acción del bot** + **dato .tres**.

### Etapa 2 — El mundo
1. `MapConfig` + formas de contorno (cuadrado, círculo, hexágono...). (Algoritmos, dificultad y tamaños ✅ en v0.0.10.)
2. Áreas conectadas por puertas → **la bóveda como mini-mapa propio**; las puertas sirven para portales.
3. Terrenos como datos: agua, barro, hielo (inercia), ríos con puentes, árboles/rocas.
4. Mapa que cambia: paredes rompibles, atajos, pasadizos secretos, paredes móviles.
5. Geometría avanzada: laberintos circulares de anillos, hexagonales, paredes diagonales/curvas.

### Etapa 3 — Bots ✅ (base en v0.0.17)
Hecho: `BotController`, A* con costos, Utility AI, dificultades. Falta: personalidades (pesos por acción en .tres),
árboles de comportamiento para vándalos/guardianes, bots que usen armas/portales (una `BotAction` por mecánica).

### Etapa 4 — Combate
Salud, 3 vidas, reaparición (State), soltar el trofeo al morir; armas, bombas, minas, escudos, pociones como datos.

### Etapa 5 — Seguridad de la bóveda
Soldados guardianes (mini jefes).

### Etapa 6 — Vándalos y Director de IA
Robo, burlas, saqueo, emboscadas coordinadas; el Director controla apariciones, eventos y objetos.

### Etapa 7 — Presentación
Identidad visual (paleta, fuente, Theme, glow, Forward+), avatares, arte, sonido, partículas, cámara (Phantom Camera).

### Después
Demo en itch.io · aliados · torneo con rondas · multijugador local · en línea · Steam (VISION §17).
## Ideas anotadas (no implementar todavía)

- **Política del Director para objetos:** el Director de IA (fase 5) tendrá su propia `PickupSpawnPolicy` (p. ej. más objetos cerca de quien va perdiendo).
- **Más tipos de objeto** (pociones, escudos, bombas): solo nuevos .tres + los efectos que falten en la fase de combate.

- **Multijugador local y visión:** solo hay una luz/oscuridad por pantalla; con varios humanos en la misma máquina hará falta pantalla dividida u otra solución.
- **Partes de cada fase en el menú:** hoy se ajustan en `data/match/default_rules.tres`; se podría exponer en "Personalizado".
- **Bóveda proporcional al pasillo:** el radio de la bóveda está en casillas; con pasillos de 5, una bóveda de radio 2 mide lo mismo que un pasillo. Podría medirse en celdas.
- **Qué pasa en cada fase:** vándalos y guardianes por fase (Etapas 5–6), bases que se mueven en el Golpe.
- **Ideas del dueño del proyecto (en orden, una pieza a la vez):** botones de acción del participante (disparar,
  usar objeto) como nuevos campos de `ParticipantCommand`; power-ups nuevos (.tres); romper paredes; portales
  (puertas de la Etapa 2.2); objetos que al comerlos cambian el mapa y los caminos (usan `Maze.queue_passage_change`).
  Cada mecánica trae su `BotAction` para que los bots también la usen.
- **Choques entre participantes:** con el combate (Etapa 4). Hoy se atraviesan. El robo básico por contacto ya existe (v0.0.18).
- **Bots que huyen con el trofeo** (esquivar a quien los persigue): una `BotAction` más cuando haga falta.
