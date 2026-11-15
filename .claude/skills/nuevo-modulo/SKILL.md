---
name: nuevo-modulo
description: Flujo obligatorio para construir una pieza o módulo nuevo de Laberinto Vándalo (por ejemplo "puntos recolectables", "trofeo", "bots", "vándalos"), o para cambiar uno existente de forma importante. Guía diseño con patrones → aprobación del usuario → lógica en core con tests → nodo de Godot → sandbox → verificación → documentación → commit. Usar cuando el usuario pida avanzar con la siguiente pieza del ROADMAP o agregar un sistema/mecánica nueva.
---

# /nuevo-modulo

El proyecto se construye **una pieza a la vez** y **primero el diseño**. Esta skill es el procedimiento.
Leer antes: `CLAUDE.md`, `docs/ROADMAP.md` (qué sigue) y la sección del módulo en `docs/VISION.md`.

## Fase A — Diseño (sin código)

1. Identificar la pieza en `docs/ROADMAP.md` y lo que dice `docs/VISION.md` sobre ella.
2. Revisar qué existe y se puede reutilizar (la última sección de `docs/ARQUITECTURA.md` es el índice de patrones).
3. Escribir la propuesta con [`plantilla-diseno.md`](plantilla-diseno.md): problema, solución,
   patrones **con su porqué**, clases, qué va en `core` y qué en `game`, cómo se prueba en pantalla.
   - Un patrón solo si resuelve un problema de esta pieza o de una fase cercana ya planificada. Si es la
     versión "de juegos" de un patrón clásico, decirlo con honestidad.
   - Lenguaje simple: el usuario no es experto en C++.
4. **Detenerse y pedir aprobación.** No escribir código hasta que el usuario apruebe (puede ser con un
   "ok", "dale", "vamos"). Si pide cambios, ajustar el diseño y volver a mostrarlo.
   Si el usuario ya aprobó explícitamente en el mismo mensaje, continuar.

## Fase B — Implementación (en este orden)

1. **`src/core/`**: lógica pura C++17, namespace `lv::core`, sin includes de Godot.
2. **`tests/test_<modulo>.cpp`**: tests doctest de esa lógica (casos normales, bordes y lo que podría romperse).
   Correr `/verificar -SinGodot` hasta que pasen.
3. **`src/game/`**: nodo/recurso de Godot que **adapta** la lógica de core (no la duplica).
   - Comentario de cabecera: qué es, qué patrón aplica y `Ver docs/ARQUITECTURA.md §N`.
   - Propiedades editables con `ADD_PROPERTY`, señales con `ADD_SIGNAL`, nada visual dentro del nodo lógico.
   - Registrar en `src/register_types.cpp` (las clases base antes que sus derivadas;
     `GDREGISTER_ABSTRACT_CLASS` para las abstractas).
4. **Escena/vista** en `project/` (GDScript solo para vista e interfaz) y datos en `project/data/` si aplica.
5. **Sandbox**: agregar la pieza a la escena de prueba actual (o crear `scenes/sandbox/<modulo>_sandbox.tscn`)
   con teclas de prueba documentadas en la línea de ayuda.
6. `clang-format -i` sobre los `.cpp/.h` tocados.

## Fase C — Verificación

1. `/verificar -Captura <scratchpad>` y **mirar la captura**.
2. Si la pieza tiene comportamiento (movimiento, tiempos, efectos), escribir un script temporal de
   prueba en el scratchpad (`extends SceneTree`, ejecutar con `godot --headless --path project -s <script>`)
   que mida el resultado real. No dejar esos scripts en el repo.
3. Corregir hasta que todo esté OK. Reportar con honestidad lo que se verificó y lo que no.

## Fase D — Documentación y cierre

1. `docs/ARQUITECTURA.md`: nueva sección con [`plantilla-arquitectura.md`](plantilla-arquitectura.md)
   **antes** del índice de patrones; renumerar el índice y actualizar sus filas y la tabla de módulos (§2).
   Ajustar las referencias `§N` en los comentarios del código y en `CLAUDE.md` si cambiaron.
2. `docs/ROADMAP.md`: mover la pieza a "Hecho" con su versión y lo logrado; poner la siguiente pieza en
   "Siguiente paso"; anotar ideas nuevas en "Ideas anotadas" (no implementarlas).
3. `README.md`: versión, fila de estado de la fase y controles si cambiaron.
4. Subir la versión (patch) en `src/version.h` y `project/project.godot` (`config/version`).
5. Commit semántico en español con la co-autoría del agente. Ej.: `feat(trofeo): pieza 6 - ... (v0.0.7)`.
   Hacer `git push` solo si el usuario lo pidió o ya lo autorizó para este flujo.
6. Resumen final al usuario: qué se hizo, qué patrones y por qué (tabla corta), cómo probarlo (teclas),
   qué se verificó y cuál es la siguiente pieza.
