#pragma once

#include <cstdlib>

namespace lv::core {

// Posición de una casilla en la cuadrícula del laberinto.
struct Cell {
	int x = 0;
	int y = 0;

	bool operator==(const Cell &p_other) const { return x == p_other.x && y == p_other.y; }
	bool operator!=(const Cell &p_other) const { return !(*this == p_other); }
};

// Distancia de Chebyshev: pasos entre dos casillas contando diagonales.
inline int chebyshev_distance(const Cell &p_a, const Cell &p_b) {
	const int dx = std::abs(p_a.x - p_b.x);
	const int dy = std::abs(p_a.y - p_b.y);
	return dx > dy ? dx : dy;
}

} // namespace lv::core
