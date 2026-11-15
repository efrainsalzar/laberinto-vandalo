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
| `entities.update` | Índice espacial de entidades por frame de física | `game/entity_index.cpp` |

**Agregar una medición** (C++): `#include "core/perf.h"` y al inicio del bloque `lv::core::PerfScope perf("sistema.tarea");`.
Aparece sola en el panel F3 y en los monitores. Nombre: `<sistema>.<tarea>` en minúsculas.

## Reglas de diseño para rendimiento

1. **Ninguna vista que exista en cantidad se redibuja cada frame** (objetos, enemigos...). Se dibuja una vez.
2. **Todo cambio del mapa pasa por la cola** (`Maze.queue_passage_change`): presupuesto de **tiempo** `apply_budget_ms` (0,5 ms, incluye la reconstrucción de Godot) y tope `changes_per_frame` órdenes
   por frame. Ráfagas de cambios (jugadores, power-ups, Director) se reparten en varios frames.
3. **Costo acotado**: los algoritmos que corren durante la partida tienen un límite de trabajo (p. ej. la búsqueda
   de conectividad local se corta a los 4096 pasos y responde de forma conservadora).
4. **Nada de recorrer todo el mapa en caliente**: muestreo aleatorio, índices espaciales (`PickupFactory` casilla → objeto),
   datos que no cambian precalculados al generar (pasajes protegidos).
5. **Lógica pesada en C++** (`core`), GDScript solo para interfaz y conexión.
6. Si algo sigue costando demasiado: **trabajo en segundo plano** (WorkerThreadPool) sobre copias de los datos de `core`.

## Escalabilidad: reglas para todo lo que ocurra en tiempo real

El juego va a tener **muchas cosas a la vez**: hasta 8 participantes, bots y vándalos con IA, power-ups, armas,
el Director, jugadores que cambian el mapa, efectos visuales. Estas reglas son **obligatorias** para cualquier
sistema nuevo que corra durante la partida.

### 1. Presupuesto por área (por frame, a 60 FPS = 16,7 ms en total)

| Área | Presupuesto | Notas |
|---|---|---|
| Física y movimiento (Godot) | 3 ms | Cuerpos, colisiones |
| IA (bots, vándalos, soldados, Director) | 3 ms | Repartida entre agentes; ver regla 6 |
| Mapa vivo (todas las fuentes de cambios) | 1 ms | Cola con presupuesto (regla 2) |
| Juego (trofeo, objetos, efectos, reglas) | 1 ms | |
| Interfaz y efectos visuales | 2 ms | HUD, textos, partículas |
| Dibujo (Godot) | 4 ms | |
| **Margen** | 2,7 ms | Para picos y equipos más lentos |

El panel F3 muestra el costo real de cada sistema; si un área supera su presupuesto, se optimiza antes de seguir.

### 2. Todo cambio en tiempo real es una orden y pasa por una cola con presupuesto
- Cambios del mapa → `Maze.queue_passage_change(a, b, open, prioridad, fuente)` con `MapChangeQueue` (prioridades, reservas, tope 64). Futuras colas por dominio (efectos, apariciones) siguen el mismo modelo.
- Una ráfaga (10 power-ups a la vez, el Director moviendo media mapa) **nunca** se procesa en un solo frame: se reparte.

### 3. Límites máximos y qué hacer al superarlos
Cada cosa que puede multiplicarse tiene un **tope** y una **política** al llegar a él:

| Elemento | Tope inicial | Al superarlo |
|---|---|---|
| Participantes | 8 | No se admiten más |
| Vándalos activos | 24 | El Director espera a que haya lugar |
| Objetos en el mapa | 600 | La política de aparición no crea más |
| Efectos activos por participante | 16 | Se reemplaza el más antiguo del mismo tipo (o se renueva su duración) |
| Órdenes de cambio de mapa en cola | 64 | Se descartan las de menor prioridad |
| Textos flotantes | 20 (pool) | Se recicla el más antiguo |
| Partículas visibles | 2000 | Se reduce la cantidad por efecto |

### 4. Conflictos: varias fuentes cambiando lo mismo a la vez
- **Reserva**: un cambio pendiente (con aviso en curso) **reserva** su pasaje; otra fuente no puede tocarlo hasta que termine.
- **Prioridad** cuando dos órdenes compiten: seguridad (rescates) > acciones de jugadores y power-ups > Director > laberinto vivo automático.
- **Las garantías se verifican al aplicar, no al pedir**: una orden que ya no es segura (p. ej. dejaría a alguien encerrado) se descarta.

### 5. Reutilizar en lugar de crear y destruir (Object Pool)
Todo lo que aparece y desaparece seguido (textos, proyectiles, partículas, efectos, vándalos) usa un pool.

### 6. Actualizar según importancia (LOD de lógica)
Lo lejano o fuera de la vista se actualiza menos: un vándalo lejos piensa 4 veces por segundo, uno cerca cada frame.
Los efectos visuales fuera de pantalla no se dibujan.

### 7. Nunca recorrer todo en caliente
> Lección (v0.0.14): un sistema barato por llamada pero que **recorre todo en cada frame** fue el más caro de la prueba de estrés (`pickups.spawner`, 21,9 ms/s). Mirar siempre `ms/s`, no solo `peor`. Contadores mantenidos en lugar de contar.
Consultas como "¿quién está cerca?" o "¿qué hay en esta casilla?" usan **índices espaciales** (cuadrícula de cubetas),
no recorridos de todas las entidades. Los datos que no cambian se precalculan.

### 8. Eventos agrupados
Un sistema que produce muchos eventos por frame los **agrupa** (una señal con la lista) en lugar de emitir uno por elemento.

### 9. Trabajo pesado fuera del hilo principal
Cálculos largos y puros (caminos de muchos bots, planificación del Director) van a `WorkerThreadPool` sobre copias de
los datos de `core`; el hilo principal solo aplica resultados.

### 10. Cada etapa se cierra con una prueba de estrés
Menú principal › **Prueba de estrés** (30 s, VSync apagado). Veredicto: **dentro del presupuesto** si p99 ≤ 16,7 ms y peor ≤ 33 ms.
Al agregar entidades o sistemas nuevos, se suman a la carga de la prueba (`scripts/game/stress_test.gd`).
Escenario de referencia (Gigante, Difícil, Caótico) **más la carga máxima de la etapa** (p. ej. 8 bots + 24 vándalos).
El peor frame debe quedar dentro del presupuesto antes de dar la etapa por terminada.

### Checklist para una pieza nueva en tiempo real
- [ ] ¿Tiene `PerfScope` y está en la tabla de sistemas medidos?
- [ ] ¿Sus cambios pasan por una cola con presupuesto?
- [ ] ¿Tiene tope y política al superarlo?
- [ ] ¿Qué pasa si dos fuentes la activan a la vez? (reserva / prioridad)
- [ ] ¿Usa pool si aparece y desaparece seguido?
- [ ] ¿Se actualiza menos cuando está lejos o fuera de la vista?
- [ ] ¿Evita recorrer todo (usa índice espacial)?
- [ ] ¿Pasó la prueba de estrés dentro del presupuesto?

## Historial de mediciones

| Fecha / versión | Escenario | Resultado |
|---|---|---|
| v0.0.11 | Gigante Difícil, sin VSync | ~70 FPS → **~450 FPS** al dejar de redibujar los objetos cada frame |
| v0.0.13 | Gigante Difícil, laberinto Caótico, en movimiento | **Sin tirones** (prueba en vivo del dueño del proyecto) tras la cola de órdenes y la búsqueda local acotada |
| v0.0.14 | **Prueba de estrés** (40 entidades + ráfagas) | **FUERA**: promedio 8,0 ms, p99 14,3, peor 134,6 ms. Más caro: `pickups.spawner` 21,9 ms/s (contaba los 468 objetos cada frame), `maze.apply`+`maze.rebuild` ~1,5 ms en picos. |
| v0.0.14 (arreglos) | Prueba de estrés | Contador O(1) de objetos y separación por índice espacial; aplicación de cambios con presupuesto de **tiempo** (0,5 ms) que incluye la reconstrucción de Godot; bloques de paredes 8×8; calentamiento de 2 s excluido e instante del peor frame. *Pendiente: nueva medición en vivo.* |
