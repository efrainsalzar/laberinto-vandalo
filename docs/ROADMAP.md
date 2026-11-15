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

## Plan acordado (bloques, uno a la vez)

1. ~~Objetos infinitos~~ ✅
2. ~~HUD que no tapa el juego + textos flotantes~~ ✅
3. **Identidad visual** (Theme, fuente, paleta; recomendado: neón arcade con glow, renderer Forward+; Phantom Camera). ← siguiente
4. Pieza 6 — Trofeo y zonas de resguardo.
5. Pieza 7 — Partida: reloj, fases (State), victoria y fin por tiempo.
6. Menús: principal, pausa, fin de partida; flujo del juego (State).

## Después (fase 1 — prototipo), un módulo a la vez

6. Trofeo y zonas de resguardo.
7. Gestor de partida: reloj, victoria y fin por tiempo.
8. Arte CC0 (Kenney) reemplazando las formas de colores.

## Ideas anotadas (no implementar todavía)

- **Política del Director para objetos:** el Director de IA (fase 5) tendrá su propia `PickupSpawnPolicy` (p. ej. más objetos cerca de quien va perdiendo).
- **Más tipos de objeto** (pociones, escudos, bombas): solo nuevos .tres + los efectos que falten en la fase de combate.

- **Multijugador local y visión:** solo hay una luz/oscuridad por pantalla; con varios humanos en la misma máquina hará falta pantalla dividida u otra solución.

