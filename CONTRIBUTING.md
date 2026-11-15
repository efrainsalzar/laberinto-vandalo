# Cómo contribuir

¡Gracias por querer ayudar con **Laberinto Vándalo**! El proyecto está en una etapa temprana, así que
lo más valioso ahora es seguir el plan paso a paso.

## Antes de empezar

1. Lee [`docs/VISION.md`](docs/VISION.md) (qué juego es) y [`docs/ROADMAP.md`](docs/ROADMAP.md) (en qué estamos).
2. Lee [`CLAUDE.md`](CLAUDE.md): tiene la arquitectura, las convenciones y los comandos.
3. Compila el proyecto siguiendo el [README](README.md#compilar-y-ejecutar).

## Flujo de trabajo

1. Busca un *issue* (los marcados `good first issue` son ideales para empezar) o abre uno para proponer algo.
   **Para cambios grandes, primero discute el diseño en el issue.**
2. Haz un *fork* y crea una rama: `feat/nombre-corto` o `fix/nombre-corto`.
3. Haz commits pequeños con formato semántico:
   - `feat:` funcionalidad nueva · `fix:` corrección · `docs:` documentación
   - `test:` tests · `refactor:` reorganización sin cambiar comportamiento · `build:` compilación/CI · `chore:` mantenimiento
4. Antes de abrir el *pull request*:
   - En Windows: `./tools/verificar.ps1` termina con todo en **OK** (compila, tests y Godot sin errores).
   - En otros sistemas: `scons target=template_debug` compila y `scons tests=yes` pasa.
   - El código está formateado: `clang-format -i` sobre los archivos que tocaste.
   - Probaste el cambio en Godot.
5. Abre el *pull request* explicando **qué** cambia y **por qué**.

## Reglas del proyecto

- **Todo original.** No se aceptan nombres, arte, sonidos ni apariencia copiados de juegos o franquicias existentes.
- Arte o sonido de terceros: solo con licencia compatible (preferiblemente **CC0**) y anotado en [`CREDITS.md`](CREDITS.md).
- La lógica del juego va en C++ (`src/core` si no depende de Godot); GDScript solo para interfaz y escenas.
- Documentación y comentarios en español; nombres de código en inglés.

## Reportar errores

Abre un *issue* con: qué hiciste, qué esperabas, qué pasó, tu sistema operativo y versión de Godot.
