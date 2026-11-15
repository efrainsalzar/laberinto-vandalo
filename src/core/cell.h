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

// Rectángulo de casillas (x, y = esquina superior izquierda; w, h = tamaño en casillas).
// Se usa para zonas que ocupan varias casillas, como las bases (miden lo que mide el pasillo).
struct TileRect {
	int x = 0;
	int y = 0;
	int w = 1;
	int h = 1;

	bool contains(const Cell &p_cell) const { return p_cell.x >= x && p_cell.y >= y && p_cell.x < x + w && p_cell.y < y + h; }
	// Casilla central (en tamaños pares, la de abajo a la derecha del centro real).
	Cell center() const { return { x + w / 2, y + h / 2 }; }
	// Cuadro de (2r+1)x(2r+1) casillas alrededor de una casilla.
	static TileRect around(const Cell &p_cell, int p_radius) {
		const int r = p_radius < 0 ? 0 : p_radius;
		return { p_cell.x - r, p_cell.y - r, 2 * r + 1, 2 * r + 1 };
	}
	bool operator==(const TileRect &p_o) const { return x == p_o.x && y == p_o.y && w == p_o.w && h == p_o.h; }
};

// Distancia de Chebyshev: pasos entre dos casillas contando diagonales.
inline int chebyshev_distance(const Cell &p_a, const Cell &p_b) {
	const int dx = std::abs(p_a.x - p_b.x);
	const int dy = std::abs(p_a.y - p_b.y);
	return dx > dy ? dx : dy;
}

} // namespace lv::core
