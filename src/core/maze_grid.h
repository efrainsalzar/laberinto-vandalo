#pragma once

#include "core/cell.h"

#include <cstdint>
#include <random>
#include <vector>

namespace lv::core {

// Laberinto como cuadrícula de casillas (pared o suelo). Lógica pura, sin Godot.
//
// Se generan `cells_w x cells_h` celdas lógicas; la cuadrícula real mide
// (2 * cells_w + 1) x (2 * cells_h + 1): celdas en coordenadas impares y paredes entre ellas.
// Ver docs/ARQUITECTURA.md §4.
class MazeGrid {
public:
	struct Options {
		int cells_w = 12;
		int cells_h = 9;
		uint32_t seed = 1;
		float loop_chance = 0.12f; // Probabilidad de abrir paredes extra (0 = un solo camino).
		int vault_radius = 2; // Sala central de (2r+1)x(2r+1). 0 = sin sala.
	};

	// Genera un laberinto nuevo. Misma semilla y opciones => mismo laberinto.
	void generate(const Options &p_options);

	int width() const { return width_; }
	int height() const { return height_; }

	bool in_bounds(const Cell &p_cell) const;
	bool is_wall(const Cell &p_cell) const; // Fuera de los límites cuenta como pared.

	std::vector<Cell> floor_cells() const;
	Cell center() const;
	bool is_in_vault(const Cell &p_cell) const;

	// Casillas de inicio (zonas de resguardo): primero las 4 esquinas, luego los bordes. Máximo 8.
	std::vector<Cell> base_cells(int p_count) const;

	// true si se puede llegar a todo el suelo desde cualquier casilla de suelo.
	bool is_fully_connected() const;

private:
	int width_ = 0;
	int height_ = 0;
	int vault_radius_ = 0;
	std::vector<uint8_t> walls_;

	int index(const Cell &p_cell) const { return p_cell.y * width_ + p_cell.x; }
	void set_wall(const Cell &p_cell, bool p_wall);

	// Pasos de la generación (en orden).
	void carve_perfect_maze(int p_cells_w, int p_cells_h, std::mt19937 &r_rng);
	void open_loops(float p_chance, std::mt19937 &r_rng);
	void open_vault();
};

} // namespace lv::core
