# 🏆 Laberinto Vándalo

[![CI](https://github.com/efrainsalzar/laberinto-vandalo/actions/workflows/ci.yml/badge.svg)](https://github.com/efrainsalzar/laberinto-vandalo/actions/workflows/ci.yml)
[![Licencia: MIT](https://img.shields.io/badge/licencia-MIT-blue.svg)](LICENSE)

> Nombre provisional · **En desarrollo temprano (v0.0.30)**

**Un torneo en un laberinto vivo donde varios participantes compiten por un trofeo, pero unos vándalos
sabotean todo para robárselo. Solo gana uno: quien consiga el trofeo y vuelva con él a su zona de
resguardo antes de que se acabe el tiempo.**

Juego 2D top-down hecho con **Godot 4** y **C++ (GDExtension)**. Código abierto bajo licencia MIT.

📖 La visión completa del juego está en [`docs/VISION.md`](docs/VISION.md).

---

## Estado

| Fase | Estado |
|---|---|
| Base del proyecto (entorno, compilación, tests, CI, documentación) | ✅ Lista |
| Diseño de arquitectura ([ARQUITECTURA.md](docs/ARQUITECTURA.md)) | ✅ Lista |
| Fase 1 — Prototipo: laberinto ✅ · participante ✅ · stats ✅ · visión ✅ · objetos ✅ (infinitos) · HUD ✅ · trofeo y bases ✅ · mapas con 6 algoritmos y 4 dificultades ✅ · menús ✅ · laberinto vivo ✅ · partida (reloj y fases) ✅ · base | 🚧 En curso |
| Bots con 4 dificultades ✅ (v0.0.17) |  ✅ |
| Disparo + pistola inicial (humano y bots) ✅ (v0.0.23) | ✅ |
| Armas del mapa: escopeta, láser y bazuca + HUD de armas ✅ (v0.0.26) | ✅ |
| Combate · Soldados · Vándalos · Mapa vivo | 🔜 |

Detalle y siguiente paso: [`docs/ROADMAP.md`](docs/ROADMAP.md) · historial: [`docs/HISTORIAL.md`](docs/HISTORIAL.md) · cómo pedir tareas al agente: [`docs/COMO_PEDIR.md`](docs/COMO_PEDIR.md).

---

## Tecnología

| Área | Herramienta |
|---|---|
| Motor | [Godot 4.7](https://godotengine.org) (versión estándar, no .NET) |
| Lógica | C++17 con [godot-cpp](https://github.com/godotengine/godot-cpp) 10 (GDExtension) |
| Interfaz | GDScript |
| Compilación | [SCons](https://scons.org) |
| Tests | [doctest](https://github.com/doctest/doctest) |
| Calidad | clang-format, clang-tidy |

---

## Compilar y ejecutar

### Requisitos

- **Godot 4.3 o superior** (recomendado 4.7)
- **Git**
- **Python 3.8+** y **SCons**: `pip install scons`
- **Compilador C++**
  - Windows: Visual Studio 2022 (o Build Tools) con "Desarrollo para el escritorio con C++"
  - Linux: GCC o Clang
  - macOS: Xcode Command Line Tools

### Pasos

```bash
# 1. Clonar con el submódulo godot-cpp
git clone --recurse-submodules https://github.com/efrainsalzar/laberinto-vandalo.git
cd laberinto-vandalo

# 2. Compilar la extensión C++ (la primera vez tarda varios minutos)
scons target=template_debug

# 3. (Opcional) Ejecutar los tests unitarios
scons tests=yes

# 4. Abrir el proyecto en Godot
godot --editor --path project
```

Si ya clonaste sin `--recurse-submodules`: `git submodule update --init --recursive`.

Al ejecutar verás el **menú principal**: en **Jugar** eliges la dificultad del mapa (con vista previa), cuántos **rivales (bots)** y su dificultad, y empiezas. Toma el **trofeo** del centro (si otro lo lleva, **tócalo para atraparlo**: vuelve a su base y el trofeo cae al piso) y llévalo a **tu base** antes de que se acabe el **reloj** (la flecha del borde te guía); la partida pasa por tres fases y en la última el laberinto se acelera. Muévete con **WASD** o las flechas, apunta con el **mouse** y dispara con **clic izquierdo** (un balazo manda al rival a su base y le hace soltar el trofeo), y recoge objetos. **Esc** pausa, **F1** muestra los controles y **F3** el panel de rendimiento.

---

## Estructura

```
src/         Código C++ (core = lógica pura, game = nodos de Godot)
tests/       Tests unitarios (doctest)
project/     Proyecto de Godot (escenas, scripts, assets)
docs/        Visión, hoja de ruta y decisiones de diseño
godot-cpp/   Submódulo con los bindings de Godot
```

---

## Contribuir

¡Las contribuciones son bienvenidas! Lee [`CONTRIBUTING.md`](CONTRIBUTING.md).
Si usas un agente de IA para programar, que lea primero [`CLAUDE.md`](CLAUDE.md).

## Licencia

- **Código:** [MIT](LICENSE).
- **Nombre del juego, personajes y arte original:** reservados (por definir).
- **Recursos de terceros:** ver [`CREDITS.md`](CREDITS.md).
