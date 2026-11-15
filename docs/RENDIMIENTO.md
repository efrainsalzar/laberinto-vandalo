# Rendimiento

> Regla del proyecto: **medir antes de optimizar, y cada sistema nuevo trae su medición.**

## Presupuesto

| Medida | Objetivo | Límite |
|---|---|---|
| Frame (promedio) | ≤ 8 ms | 16,7 ms (60 FPS) |
| **Peor frame del último segundo** (tirones) | ≤ 16,7 ms | 33 ms (nunca más) |
| Un sistema nuestro, por llamada (`peor` en F3) | ≤ 1 ms | 4 ms |
| Un cambio del laberinto vivo (`maze.plan` + `maze.apply` + `maze.rebuild` en un frame) | ≤ 2 ms | 5 ms |

Escenario de referencia para medir: **mapa Gigante, Difícil, laberinto vivo Caótico**, moviéndose sin parar.

## Cómo medir

1. **Panel F3** (o Opciones › Panel de rendimiento), en cualquier pantalla:
   - `FPS · frame · peor`: el **peor** en amarillo (> 16,7 ms) o rojo (> 33 ms) indica tirones.
   - Gráfico de los últimos 180 frames con líneas de 60 y 30 FPS: los picos se ven como barras altas.
   - Tiempos de Godot (proceso, física), llamadas de dibujo y nodos.
   - **Nuestros sistemas**: `ms/s` (costo total por segundo), `peor` (la llamada más cara) y `llamadas`.
2. **Depurador de Godot** (al ejecutar desde el editor): pestaña *Monitores* → grupo **Laberinto**
   (`peor_frame_ms` y `<sistema>_max_ms`), con historial graficado.
3. **Desactivar VSync** (Opciones) para ver el rendimiento real; con VSync el FPS se limita al monitor.

## Sistemas medidos

| Nombre | Qué mide | Dónde |
|---|---|---|
| `maze.plan` | Planificar un cambio del laberinto vivo | `core/maze_shift.cpp` |
| `maze.shifter` | Lógica del laberinto vivo por frame (avisos, ocupación) | `game/maze_shifter.cpp` |
| `maze.apply` | Aplicar órdenes de la cola de cambios (incluye `maze.rebuild`) | `game/maze.cpp` |
| `maze.rebuild` | Reconstrucción de colisión y sombras de Godot tras cambiar casillas | `game/maze.cpp` |
| `pickups.spawner` | Reaparición de objetos por frame | `game/pickup_spawner.cpp` |
| `pickups.populate` | Colocar los objetos iniciales | `game/pickup_factory.cpp` |
| `pickups.remove` | Retirar objetos tapados por una pared | `game/pickup_factory.cpp` |
| `trophy.update` | Reglas del trofeo por frame de física | `game/trophy_controller.cpp` |

**Agregar una medición** (C++): `#include "core/perf.h"` y al inicio del bloque `lv::core::PerfScope perf("sistema.tarea");`.
Aparece sola en el panel F3 y en los monitores. Nombre: `<sistema>.<tarea>` en minúsculas.

## Reglas de diseño para rendimiento

1. **Ninguna vista que exista en cantidad se redibuja cada frame** (objetos, enemigos...). Se dibuja una vez.
2. **Todo cambio del mapa pasa por la cola** (`Maze.queue_passage_change`): presupuesto de `changes_per_frame` órdenes
   por frame. Ráfagas de cambios (jugadores, power-ups, Director) se reparten en varios frames.
3. **Costo acotado**: los algoritmos que corren durante la partida tienen un límite de trabajo (p. ej. la búsqueda
   de conectividad local se corta a los 4096 pasos y responde de forma conservadora).
4. **Nada de recorrer todo el mapa en caliente**: muestreo aleatorio, índices espaciales (`PickupFactory` casilla → objeto),
   datos que no cambian precalculados al generar (pasajes protegidos).
5. **Lógica pesada en C++** (`core`), GDScript solo para interfaz y conexión.
6. Si algo sigue costando demasiado: **trabajo en segundo plano** (WorkerThreadPool) sobre copias de los datos de `core`.

## Historial de mediciones

| Fecha / versión | Escenario | Resultado |
|---|---|---|
| v0.0.11 | Gigante Difícil, sin VSync | ~70 FPS → **~450 FPS** al dejar de redibujar los objetos cada frame |
| v0.0.13 | Gigante Difícil, laberinto Caótico | *pendiente: prueba en vivo con F3* |
