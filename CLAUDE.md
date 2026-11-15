# CLAUDE.md — Guía para agentes de IA (y colaboradores humanos)

Este archivo es el contexto que un agente (Claude Code u otro) debe leer antes de tocar el proyecto.
También sirve como guía rápida para personas.

## Qué es este proyecto

**Laberinto Vándalo** (nombre provisional): juego 2D top-down. Un torneo en un laberinto vivo donde
varios participantes compiten por un trofeo mientras unos vándalos sabotean todo para robárselo.
Gana quien lleva el trofeo a su zona de resguardo antes de que acabe el tiempo.

- Visión completa del juego: [`docs/VISION.md`](docs/VISION.md) — **fuente de verdad del diseño.**
- Arquitectura y patrones: [`docs/ARQUITECTURA.md`](docs/ARQUITECTURA.md).
- Estado y siguiente paso: [`docs/ROADMAP.md`](docs/ROADMAP.md).

## Forma de trabajar (OBLIGATORIO)

1. **Paso a paso.** Se construye un módulo a la vez. Nunca implementar varias fases o sistemas de golpe.
2. **Primero diseño, después código.** Antes de escribir un módulo nuevo: explicar qué clases, qué patrón
   y por qué, y **esperar la aprobación del dueño del proyecto.**
3. **Nada fuera de alcance.** Si algo no está pedido en el paso actual, se anota en `docs/ROADMAP.md`, no se implementa.
4. **Cada módulo termina verificado:** compila, sus tests pasan y se ve funcionando en Godot.
5. **El idioma del proyecto es español** (documentos, comentarios, mensajes de commit). Los identificadores de código van en inglés.
6. **Commits pequeños** con formato semántico: `feat:`, `fix:`, `docs:`, `build:`, `test:`, `refactor:`, `chore:`.

## Arquitectura (resumen)

- **C++ (GDExtension, godot-cpp)** para la lógica: laberinto, reglas, IA, sistemas.
- **GDScript** solo para interfaz y para conectar escenas.
- **La lógica pura no depende de Godot** (`src/core/`): se prueba con doctest sin abrir el motor.
- **Composición sobre herencia.** Patrones previstos (ver VISION §15): Strategy (controladores),
  State (estados/fases), Command (acciones), Observer (señales), Factory + datos, Decorator (modificadores), Composite (IA).
- **Idea central: el participante no sabe quién lo controla** (humano, bot o red).
- Sin sobreingeniería: no se diseña para "cambiar de motor". Se aplica un patrón cuando resuelve un problema real.
- **Todo módulo nuevo se documenta en `docs/ARQUITECTURA.md`**: problema, patrón elegido y por qué,
  diagrama de clases (Mermaid), archivos y decisiones pendientes. Y se agrega al índice de patrones (última sección).
- Lógica y vista separadas: los nodos C++ no dibujan; la vista (escena/GDScript) no decide.
- **Eventos de juego por el Event Bus** (`Events`, autoload): los niveles publican ahí lo que ocurrió;
  la interfaz, los sonidos y el Director escuchan ahí. La lógica C++ no conoce la interfaz.
- **Interfaz:** el centro de la pantalla queda libre; HUD en los bordes; ayuda en F1; textos flotantes para feedback.

## Estructura de carpetas

```
src/
  register_types.cpp   Registro de clases C++ en Godot (único punto de registro)
  version.h            Versión del proyecto
  core/                Lógica pura C++ (sin Godot) — testeable con doctest
  game/                Clases que Godot ve (nodos, recursos) — envuelven a core/
tests/                 Tests unitarios (doctest) de src/core
thirdparty/            Dependencias incluidas (doctest)
godot-cpp/             Submódulo: bindings C++ de Godot (no editar)
project/               Proyecto de Godot (escenas, scripts GDScript, assets)
  bin/                 laberinto.gdextension + DLL compiladas (las DLL no se suben)
  scenes/sandbox/      Escenas de prueba por módulo (no son parte del juego final)
docs/                  Visión, arquitectura, hoja de ruta
```

## Comandos

Entorno local (Windows): activar con `. D:\Github\gamedev\env.ps1` (scons, clang-format, godot en el PATH).

| Acción | Comando |
|---|---|
| Compilar extensión (debug) | `scons platform=windows target=template_debug compiledb=yes` |
| Tests unitarios | `scons tests=yes` |
| Ejecutar juego | `godot --path project` |
| Ejecutar sin ventana (verificación) | `godot --headless --path project --quit-after 5` |
| Formatear C++ | `clang-format -i src/**/*.cpp src/**/*.h tests/*.cpp` |
| **Verificar todo** (compilar + tests + Godot) | `./tools/verificar.ps1` (opciones: `-SinCompilar`, `-SinGodot`, `-Captura <carpeta>`, `-Escena res://...`) |

Después de cambiar C++ hay que **recompilar y reabrir el editor de Godot** (la extensión no se recarga sola).

## Skills del proyecto (`.claude/skills/`)

| Skill | Cuándo |
|---|---|
| `/nuevo-modulo` | Construir la siguiente pieza del ROADMAP o un sistema nuevo: diseño → aprobación → core + tests → Godot → sandbox → verificar → documentar → commit. Incluye plantillas. |
| `/verificar` | Antes de cada commit o tras cualquier cambio: ejecuta `tools/verificar.ps1` y reporta con honestidad. |

Las skills describen el **proceso**; la lógica ejecutable vive en `tools/` para que también la usen personas y CI.
Si se repite una tarea varias veces, se convierte en skill (y su parte ejecutable, en un script de `tools/`).

## Convenciones de código C++

- Estilo de godot-cpp: `.clang-format` del repo (tabs, llaves en la misma línea).
- Clases en `PascalCase`, métodos y variables en `snake_case`.
- Parámetros con prefijo `p_`, miembros privados con sufijo `_` en `core/`.
- Lógica pura en el namespace `lv::core`. Clases de Godot sin namespace (Godot las registra por nombre).
- Toda clase nueva de Godot se registra en `src/register_types.cpp`.
- En `core/`, lo aleatorio usa siempre `lv::core::Rng` (nunca `std::uniform_*_distribution` ni `std::shuffle`): misma semilla = mismo resultado en todas las plataformas.
- Nada de lógica de reglas dentro de nodos visuales: el nodo delega en `core/`.

## Reglas que no se rompen

- **Todo original.** No copiar nombres, arte, sonidos ni apariencia de franquicias existentes.
- Arte de prototipo solo de fuentes **CC0** (por ejemplo Kenney.nl), registrado en `CREDITS.md`.
- No subir binarios compilados ni la carpeta `.godot/`.
