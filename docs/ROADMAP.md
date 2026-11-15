# Hoja de ruta y estado

La hoja de ruta completa del juego está en [VISION.md §17](VISION.md#17-hoja-de-ruta).
Aquí se lleva el **estado real** y el **siguiente paso**.

## Estado actual: v0.0.1 — Base del proyecto ✅

- [x] Entorno de desarrollo (Godot 4.7, godot-cpp 10, SCons, MSVC, VS Code).
- [x] Proyecto creado desde godot-cpp-template; la extensión se llama `laberinto`.
- [x] La extensión C++ compila y Godot la carga (escena de arranque `boot.tscn`).
- [x] Tests unitarios con doctest (`scons tests=yes`).
- [x] Documentación: README, CLAUDE.md, VISION, CONTRIBUTING, LICENSE.
- [x] Repositorio público en GitHub con CI.

## Siguiente paso: diseño de arquitectura (sin código)

Documento `docs/ARQUITECTURA.md` con los módulos, sus responsabilidades y el patrón de cada uno.
Se revisa y aprueba antes de programar.

## Después (fase 1 — prototipo), un módulo a la vez

Orden tentativo, a confirmar en el diseño:

1. Generador de laberinto (`core`) + tests → dibujado con `TileMapLayer`.
2. Participante + controlador intercambiable (Strategy) → controlador humano.
3. Stats del participante (componente).
4. Visión limitada (niebla de guerra).
5. Puntos recolectables.
6. Trofeo y zonas de resguardo.
7. Gestor de partida: reloj, victoria y fin por tiempo.
8. Arte CC0 (Kenney) reemplazando las formas de colores.

## Ideas anotadas (no implementar todavía)

_(vacío)_
