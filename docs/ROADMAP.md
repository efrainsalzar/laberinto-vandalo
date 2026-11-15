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

## Siguiente paso: Pieza 5 — Puntos recolectables

Diseñar y aprobar antes de programar (Factory + datos; aplican modificadores de Stats).

## Después (fase 1 — prototipo), un módulo a la vez

5. Puntos recolectables.
6. Trofeo y zonas de resguardo.
7. Gestor de partida: reloj, victoria y fin por tiempo.
8. Arte CC0 (Kenney) reemplazando las formas de colores.

## Ideas anotadas (no implementar todavía)

- **Multijugador local y visión:** solo hay una luz/oscuridad por pantalla; con varios humanos en la misma máquina hará falta pantalla dividida u otra solución.

