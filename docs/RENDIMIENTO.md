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
| `pickups.spawner` | Reaparición y caducidad de objetos por frame (por categoría) | `game/pickup_spawner.cpp` |
| `pickups.populate` | Colocar los objetos iniciales | `game/pickup_factory.cpp` |
| `pickups.remove` | Retirar objetos tapados por una pared | `game/pickup_factory.cpp` |
| `trophy.update` | Reglas del trofeo por frame de física | `game/trophy_controller.cpp` |
| `entities.update` | Índice espacial de entidades por frame de física | `game/entity_index.cpp` |
| `match.tick` | Reloj y fases de la partida por frame de física | `game/match_controller.cpp` |
| `bots.think` | Decisión y movimiento de cada bot (percepción solo al pensar) | `game/bot_controller.cpp` |
| `bots.path` | Cola de caminos A* (máx. 2 por frame) | `game/bot_coordinator.cpp` |
| `bots.tokens` | Reparto de fichas de ataque (cada 0,25 s) | `game/bot_coordinator.cpp` |
| `bots.aim` | Gatillo de cada bot: rivales visibles (cada 0,15 s) y puntería | `game/bot_controller.cpp` |
| `weapons.projectiles` | Avance y choques de todas las balas + rayos y explosiones que se apagan (máx. 256 balas; no corre sin nada) | `game/projectile_system.cpp` |
| `weapons.beam` | Rayo sostenido de un láser (cada frame mientras se dispara) | `game/projectile_system.cpp` |
| `combat.damage` | Aplicar un golpe a la salud (por bala, perdigón, víctima de explosión o frame de láser) | `game/health_component.cpp` |

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

## Prueba de estrés automática y registro

| Cómo | Comando |
|---|---|
| Desde el menú | Menú principal › **Prueba de estrés** (muestra el informe en pantalla) |
| Automática + resumen | `./tools/verificar.ps1 -SinCompilar -Estres` (falla si el veredicto es FUERA) |
| Solo el juego | `godot --path project -- --stress --quit-after-stress` |

Registros (en `user://stress/`; en Windows: `%APPDATA%\Godot\app_userdata\Laberinto Vándalo\stress\`):
- `latest.json` y `stress_<fecha>.json`: informe completo: veredicto, carga, percentiles (p50/p95/p99), peor frame y
  **en qué segundo**, sistemas (ms/s, peor, llamadas/s) y **tirones**: cada frame > 33 ms se captura en el momento con
  el tiempo de cada sistema en ese frame (`PerfStats.take_frame`) y los tiempos de Godot (proceso, física, dibujos, nodos,
  cambios pendientes del mapa). Así se sabe **quién** causó cada tirón.
- `history.csv`: una línea por ejecución para comparar versiones.
- 2 s de calentamiento excluidos. VSync apagado durante la prueba. Las ejecuciones varían: repetir 2-3 veces antes de concluir.

## Estrategias y patrones de rendimiento aplicados

| Estrategia / patrón | Dónde | Efecto medido |
|---|---|---|
| **Dirty flag** (redibujar solo si algo cambió) | Objetos (se dibujan una vez), participantes (solo al cambiar la dirección) | Gigante: 70 → 450 FPS (objetos); prueba de estrés: 5,2 → 1,2 ms por frame (participantes) |
| **Agregado incremental** (contador mantenido en vez de contar) | `PickupFactory.get_active_count` | `pickups.spawner`: 21,9 → 0,16 ms/s |
| **Índice espacial** | Objetos por casilla; `SpatialHash`/`EntityIndex` | Separación de objetos y ocupación sin recorrer todo |
| **Command + cola con prioridades y presupuesto de tiempo** (amortización) | `MapChangeQueue` en `Maze` | Ráfagas repartidas; peor `maze.apply` ≈ 1,2 ms |
| **Algoritmo local y acotado** | `has_alternative_path` | Costo de cerrar una pared independiente del tamaño del mapa |
| **Caché de datos inmutables** | Pasajes protegidos | O(1) |
| **Muestreo** | `plan_maze_shift`, `pick_spawn_cell` | Costo según lo pedido, no según el mapa |
| **Object Pool** | Textos flotantes | Sin tirones al aparecer muchos |
| **Instrumentación + registro estructurado** | `PerfScope`, panel F3, JSON/CSV de la prueba | Diagnóstico con datos, no a ojo |
| **Benchmark reproducible con calentamiento** | Prueba de estrés | Veredicto objetivo contra el presupuesto |

## Historial de mediciones

| Fecha / versión | Escenario | Resultado |
|---|---|---|
| v0.0.11 | Gigante Difícil, sin VSync | ~70 FPS → **~450 FPS** al dejar de redibujar los objetos cada frame |
| v0.0.13 | Gigante Difícil, laberinto Caótico, en movimiento | **Sin tirones** (prueba en vivo del dueño del proyecto) tras la cola de órdenes y la búsqueda local acotada |
| v0.0.14 | **Prueba de estrés** (40 entidades + ráfagas) | **FUERA**: promedio 8,0 ms, p99 14,3, peor 134,6 ms. Más caro: `pickups.spawner` 21,9 ms/s (contaba los 468 objetos cada frame), `maze.apply`+`maze.rebuild` ~1,5 ms en picos. |
| v0.0.14 (arreglos) | Prueba de estrés | **DENTRO**: promedio 2,7-5,2 ms, p99 5,0-14,1, peor 6,4-17,4, 0 frames > 33 ms. `pickups.spawner` 0,16 ms/s. |
| v0.0.15 | Prueba de estrés (3 ejecuciones automáticas) | **DENTRO**: promedio **1,2 ms (~820 FPS)**, p95 1,85, p99 2,1-2,4, peor 4,7-8,2 ms, 0 tirones. Dirty flag en participantes. Lo más caro: `maze.apply` 4,3 ms/s (peor 1,2 ms), del cual `maze.rebuild` (Godot) 3,9 ms/s. |
