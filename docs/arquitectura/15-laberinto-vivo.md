<!-- Sección §15 de docs/ARQUITECTURA.md (índice). -->
# §15. Etapa 2.4 (adelantada) — Laberinto vivo: mapas que cambian en tiempo real

### Problema que resuelve el diseño
Las paredes deben abrirse y cerrarse durante la partida (VISION §7) sin romper nada: colisión, sombras,
visión, objetos, trofeo y (en el futuro) los caminos de los bots. Y sin ser injusto ni atrapar a nadie.

### Solución
1. **Un solo lugar cambia el mapa**: `MazeGrid::set_passage` (core) → `Maze.set_passage` (actualiza casillas, colisión y
   oclusores) → señal **`map_changed(tiles, now_wall)`** (**Observer**). Quien depende del mapa escucha: `PickupSpawner`
   retira objetos tapados, `TrophyController` reubica el trofeo, `Events.maze_changed` para la interfaz (y luego la IA).
2. **Topología conservada**: `MazeGrid` guarda su `PassageGrid` sincronizada con las casillas (incluida la bóveda).
3. **Planificación segura** (`plan_maze_shift`, core): N aperturas + N cierres (el laberinto cambia de forma sin volverse
   más fácil ni más difícil). Cada cierre se verifica contra la conectividad; **cualquier subconjunto de cierres es seguro**.
4. **`MazeShifter`** (**State**): `IDLE → WARNING (aviso) → APPLYING → IDLE`.

### Colisión y casos borde (decisiones)
| Caso | Comportamiento |
|---|---|
| Aviso previo | 2,5 s: casillas a cerrar parpadean en rojo (más rápido al final); a abrir, contorno celeste; aviso en el HUD. |
| Alguien encima de una pared que se cierra | El cierre **espera** (casilla + 14 px de margen). Si sigue ocupado 3 s, **se cancela**. Nunca se aplasta a nadie. |
| Aperturas | Se aplican al terminar el aviso (siempre seguras). |
| Quedar encerrado | Imposible: solo se cierran pasajes con ruta alternativa (verificado en el grafo). |
| Objeto bajo una pared que se cierra | Se retira; la política de aparición lo repone en otro lugar. |
| Trofeo en el piso bajo una pared | Se mueve a la casilla de suelo más cercana (`TrophyTracker::relocate`). |
| Zonas protegidas | Bóveda (+1), bases (+ancho de pasillo) y borde exterior nunca cambian. |
| Red de seguridad | Tras cada cierre, si un participante quedara dentro de una pared, se lo mueve al suelo más cercano. |
| Física | Godot actualiza colisión y sombras del `TileMapLayer` al cambiar las casillas; la espera por ocupación evita que un cuerpo quede dentro. |

### Configuración
`MatchConfig.live_maze`: Apagado · Lento (cada 25 s, 1 par) · Normal (15 s, 2) · Caótico (7 s, 4). En mapas grandes los pares se
multiplican por el tamaño. Opción "Laberinto vivo" en Configurar partida. Tecla de prueba **K**: mover el laberinto ya.

### Archivos
| Archivo | Qué es |
|---|---|
| `src/core/maze_grid.*` | Topología conservada, `passage_tiles`, `is_protected_passage`, `can_close/open_passage`, `set_passage`, `nearest_floor` |
| `src/core/maze_shift.{h,cpp}` | `plan_maze_shift` |
| `tests/test_maze_shift.cpp` | 7 tests: topología = casillas, abrir/cerrar exacto, 60 cambios sin desconectar en los 4 niveles, protegidos, cancelar cierres, determinismo, `nearest_floor` |
| `src/game/maze_shifter.{h,cpp}` | `MazeShifter` (State, ocupación, cancelación, rescate) |
| `project/scripts/game/maze_shift_view.gd` | Avisos y destellos (solo redibuja si hay algo que mostrar) |

### Verificación
- Tests de core: 60/60. Verificación de compilación + Godot: OK.
- Prueba en el juego: el cierre **espera** con el jugador encima (confirmado). El resto de los casos queda para la prueba en vivo del dueño del proyecto.
