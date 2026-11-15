# 🏆 Laberinto Vándalo

> Nombre provisional · **En desarrollo temprano (v0.0.4)**

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
| Fase 1 — Prototipo: laberinto ✅ · participante ✅ · stats ✅ · visión ⏳ · puntos · trofeo · base | 🚧 En curso |
| Bots · Combate · Soldados · Vándalos · Mapa vivo | 🔜 |

Detalle y siguiente paso: [`docs/ROADMAP.md`](docs/ROADMAP.md).

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
git clone --recurse-submodules <URL-del-repositorio>
cd laberinto-vandalo

# 2. Compilar la extensión C++ (la primera vez tarda varios minutos)
scons target=template_debug

# 3. (Opcional) Ejecutar los tests unitarios
scons tests=yes

# 4. Abrir el proyecto en Godot
godot --editor --path project
```

Si ya clonaste sin `--recurse-submodules`: `git submodule update --init --recursive`.

Al ejecutar verás un laberinto generado por C++ y un participante. Muévete con **WASD** o las flechas, prueba efectos con **1/2/3** (y **0** para quitarlos), **Tab** muestra el mapa completo, **R** genera otro laberinto y **Esc** sale.

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
