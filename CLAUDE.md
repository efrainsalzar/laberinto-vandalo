# CLAUDE.md — Guía para agentes de IA (y colaboradores humanos)

**Laberinto Vándalo** (nombre provisional): juego 2D top-down. Torneo en un laberinto vivo donde varios
participantes compiten por un trofeo mientras unos vándalos sabotean todo. Gana quien lleva el trofeo a su base.

## Documentos (leer solo lo que la tarea necesita)

| Documento | Para qué |
|---|---|
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | Estado actual y qué sigue. **Empezar aquí.** |
| [`docs/ARQUITECTURA.md`](docs/ARQUITECTURA.md) | Índice corto: principios, mapa de módulos, patrones. El detalle de cada módulo está en `docs/arquitectura/NN-*.md`; un `§N` en el código = ese archivo. Abrir solo el que toca. |
| [`docs/VISION.md`](docs/VISION.md) | Diseño del juego (fuente de verdad). Leer solo la sección de la pieza. |
| [`docs/RENDIMIENTO.md`](docs/RENDIMIENTO.md) | Presupuesto, medición y checklist de escalabilidad (al diseñar algo en tiempo real). |
| [`docs/HISTORIAL.md`](docs/HISTORIAL.md) | Lo ya hecho, versión por versión. Normalmente no hace falta leerlo. |

Para entender código existente: buscar con Grep y leer solo los archivos implicados, no recorrer el proyecto.

## Forma de trabajar (OBLIGATORIO)

1. **Paso a paso.** Un módulo a la vez; nunca varias fases o sistemas de golpe.
2. **Primero diseño, después código.** Explicar clases, patrón y por qué, y **esperar la aprobación del dueño.**
3. **Nada fuera de alcance.** Lo no pedido se anota en `docs/ROADMAP.md` ("Ideas anotadas"), no se implementa.
4. **Cada módulo termina verificado:** compila, tests pasan, carga en Godot. Las pruebas en vivo las hace el dueño.
5. **Idioma: español** (documentos, comentarios, commits). Identificadores de código en inglés.
6. **Commits pequeños** semánticos: `feat:`, `fix:`, `docs:`, `build:`, `test:`, `refactor:`, `chore:`.

## Arquitectura (resumen; detalle en ARQUITECTURA.md §1)

- **C++ (GDExtension)** para la lógica; **GDScript** solo para interfaz y conectar escenas.
- `src/core/`: lógica pura sin Godot (namespace `lv::core`), probada con doctest. `src/game/`: nodos que la adaptan.
- **Composición sobre herencia.** El participante no sabe quién lo controla (humano, bot o red).
- Un patrón solo cuando resuelve un problema real. Lógica y vista separadas. Eventos por `Events` (Event Bus).
- Todo sistema en tiempo real lleva `lv::core::PerfScope perf("sistema.tarea")`. Cambios del mapa: `Maze.queue_passage_change`.
- Todo módulo nuevo: un archivo `docs/arquitectura/NN-modulo.md` + fila en el índice (ver skill `/nuevo-modulo`).

## Carpetas

`src/core` lógica pura · `src/game` nodos Godot · `src/register_types.cpp` registro de clases · `tests/` doctest ·
`project/` proyecto Godot (`scenes/`, `scripts/autoload/` = Events, Settings, App, UiFx; `data/` .tres; `ui/theme.tres`) ·
`godot-cpp/` submódulo (no editar) · `tools/` scripts.

## Comandos

Entorno (Windows): `. D:\Github\gamedev\env.ps1`.

| Acción | Comando |
|---|---|
| **Verificar** (compilar + tests + Godot) | `./tools/verificar.ps1` — salida corta; `-Detalle` para todo. Otras: `-SinCompilar`, `-SinGodot`, `-Captura <dir>`, `-Escena res://...`, `-Estres` |
| Ejecutar el juego | `godot --path project` |
| Formatear C++ | `clang-format -i <archivos tocados>` |

Tras cambiar C++ hay que recompilar y reabrir el editor de Godot.
Skills: `/nuevo-modulo` (pieza nueva: diseño → aprobación → core + tests → Godot → verificar → documentar → commit) y `/verificar`.

## Convenciones C++

- `.clang-format` del repo (tabs, llaves en la misma línea). Clases `PascalCase`; métodos y variables `snake_case`.
- Parámetros `p_`; miembros privados con sufijo `_` en `core/`. Clases de Godot sin namespace, registradas en `register_types.cpp`.
- Aleatorio en `core/`: siempre `lv::core::Rng` (nunca `std::uniform_*_distribution` ni `std::shuffle`).
- Nada de reglas dentro de nodos visuales: el nodo delega en `core/`.

## Reglas que no se rompen

- **Todo original.** Nada de nombres, arte, sonidos ni apariencia de franquicias existentes.
- Arte de prototipo solo **CC0** (p. ej. Kenney.nl), registrado en `CREDITS.md`. Hoy el arte es provisional (puntos y colores); el arte real llega en la Etapa 7.
- No subir binarios compilados ni `.godot/`.
