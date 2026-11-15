#pragma once

#include "core/maze_grid.h"

namespace lv::core {

// Punto en coordenadas de casilla con decimales: (2.5, 3.5) es el centro de la casilla (2, 3).
struct GridPoint {
	float x = 0.0f;
	float y = 0.0f;
};

// ¿Hay línea de visión entre dos puntos? true si ninguna casilla de pared corta el segmento.
// Recorre las casillas que atraviesa el segmento (algoritmo DDA de Amanatides & Woo).
bool has_line_of_sight(const MazeGrid &p_grid, const GridPoint &p_from, const GridPoint &p_to);

// ¿Puede `p_from` ver a `p_to`? Dentro del radio (en casillas) y con línea de visión.
// Es la regla de visión del juego: la usan los bots para no "hacer trampa".
bool can_see(const MazeGrid &p_grid, const GridPoint &p_from, const GridPoint &p_to, float p_radius);

} // namespace lv::core
