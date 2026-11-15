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

## Siguiente paso: Módulo 2 — Participante + controladores (Strategy)

Diseñar y aprobar antes de programar (ver ARQUITECTURA §2, filas 2 y 3).

## Después (fase 1 — prototipo), un módulo a la vez

3. Stats del participante (componente + modificadores).
4. Visión limitada (niebla de guerra).
5. Puntos recolectables.
6. Trofeo y zonas de resguardo.
7. Gestor de partida: reloj, victoria y fin por tiempo.
8. Arte CC0 (Kenney) reemplazando las formas de colores.

## Ideas anotadas (no implementar todavía)

_(vacío)_
