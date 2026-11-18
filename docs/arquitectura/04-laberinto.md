<!-- Sección §4 de docs/ARQUITECTURA.md (índice). -->
# §4. Módulo 1 — Laberinto (detalle)

### `lv::core::MazeGrid`

- Cuadrícula de `width × height` casillas; cada una es **pared** o **suelo**.
- Se genera a partir de `cells_w × cells_h` celdas lógicas. Cada celda es un pasillo de
  `corridor_width × corridor_width` casillas y **las paredes siempre miden 1 casilla** (paredes delgadas, pasillos anchos).
  Tamaño final: `(cells_w·(corridor_width+1)+1) × (cells_h·(corridor_width+1)+1)`.
- **Generación por pasos** (cada paso es una función separada, fácil de cambiar o probar):
  1. *Backtracking iterativo* → laberinto "perfecto" (un solo camino entre dos puntos), en una cuadrícula compacta.
     Usa `lv::core::Rng` (aleatorio portable, ver §8) para que la semilla dé el mismo laberinto en todas las plataformas.
  2. *Bucles*: abre paredes extra con probabilidad `loop_chance` → rutas alternativas para huir y emboscar.
  3. *Ensanchar*: escala la cuadrícula compacta al ancho de pasillo final.
  4. *Bóveda*: sala abierta en el centro (donde estará el trofeo).
- **Determinista por semilla:** misma semilla = mismo laberinto (útil para tests, repeticiones y multijugador).
- Consultas: `is_wall`, `floor_cells`, `center`, `is_in_vault`, `base_cells(n)` (esquinas primero), `is_fully_connected`.

**Tests:** dimensiones correctas; borde siempre pared; todo el suelo conectado; misma semilla → mismo resultado;
bóveda abierta; bases en suelo y distintas entre sí; pasillos anchos (2, 3 y 4 casillas) conectados.

### `Maze` (nodo Godot, hereda `TileMapLayer`)

- Propiedades editables en el inspector: `cells_width` (20), `cells_height` (14), `corridor_width` (3),
  `seed` (0 = aleatoria), `loop_chance` (0.12), `vault_radius` (4). Con los valores por defecto: 81×57 casillas de 16 px.
- `generate()`: crea el `MazeGrid` y pinta las casillas. Emite la señal `generated`.
- Si no tiene `TileSet` asignado, crea uno **provisional** (dos colores, con colisión en las paredes),
  para no depender de arte todavía. Con arte de Kenney solo se asigna otro `TileSet`.
- Consultas para el resto del juego: `is_wall(cell)`, `get_center_cell()`, `get_base_cells(n)`,
  `cell_to_world(cell)`, `world_to_cell(pos)`.

### Fuera de alcance del módulo 1
Mutación en tiempo real, portales, pasadizos secretos (fase 6 "Mapa vivo").
