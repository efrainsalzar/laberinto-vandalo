#pragma once

#include "core/cell.h"
#include "core/maze_algorithms.h"
#include "core/random.h"

#include <cstdint>
#include <vector>

namespace lv::core {

// Laberinto como cuadrícula de casillas (pared o suelo). Lógica pura, sin Godot.
//
// Se generan `cells_w x cells_h` celdas lógicas. Cada celda es un pasillo de
// `corridor_width x corridor_width` casillas y las paredes miden 1 casilla, así que la
// cuadrícula final mide (cells_w * (corridor_width + 1) + 1) x (cells_h * (corridor_width + 1) + 1).
// Ver docs/ARQUITECTURA.md §4.
class MazeGrid {
public:
	struct Options {
		int cells_w = 12;
		int cells_h = 9;
		uint32_t seed = 1;
		int corridor_width = 1; // Ancho de los pasillos en casillas (las paredes siempre miden 1).
		MazeAlgorithm algorithm = MazeAlgorithm::RecursiveBacktracker;
		float growing_tree_bias = 0.5f; // solo GrowingTree: 1 = backtracker, 0 = Prim
		float loop_chance = 0.12f; // Probabilidad de abrir paredes extra (0 = un solo camino).
		float braid_chance = 0.0f; // Probabilidad de eliminar cada callejón sin salida.
		int vault_radius = 2; // Sala central de (2r+1)x(2r+1) casillas. 0 = sin sala.
	};

	// Medidas del laberinto generado, para comparar dificultades.
	struct Metrics {
		MazeAlgorithm algorithm = MazeAlgorithm::RecursiveBacktracker;
		int cells = 0; // celdas lógicas
		int dead_ends = 0; // celdas con una sola salida
		int junctions = 0; // celdas con 3 o 4 salidas (decisiones)
		float dead_end_ratio = 0.0f;
		int route_length = 0; // casillas a recorrer desde la base 0 hasta el centro (camino más corto)
	};

	// Genera un laberinto nuevo. Misma semilla y opciones => mismo laberinto.
	void generate(const Options &p_options);

	int width() const { return width_; }
	int corridor_width() const { return corridor_width_; }
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

	const Metrics &metrics() const { return metrics_; }
	// Largo del camino más corto entre dos casillas de suelo (-1 si no hay camino).
	int path_length(const Cell &p_from, const Cell &p_to) const;

private:
	int width_ = 0;
	int height_ = 0;
	int vault_radius_ = 0;
	int corridor_width_ = 1;
	std::vector<uint8_t> walls_;
	Metrics metrics_;

	int index(const Cell &p_cell) const { return p_cell.y * width_ + p_cell.x; }
	void set_wall(const Cell &p_cell, bool p_wall);

	// Pasos de la generación (en orden): topología con el algoritmo elegido (+ bucles + braid)
	// → se dibuja en una cuadrícula compacta (pasillos de 1) → se ensancha → bóveda.
	void rasterize(const PassageGrid &p_passages);
	void widen_corridors(int p_corridor_width);
	void open_vault();

	// Convierte una coordenada de la cuadrícula compacta al centro de su bloque en la final.
	int compact_to_final(int p_compact) const;
};

// Niveles de dificultad predefinidos (fuente única de sus valores). Ver docs/ARQUITECTURA.md §12.
enum class MazeDifficulty : int {
	Easy = 0,
	Normal = 1,
	Hard = 2,
	Extreme = 3,
	Count
};

const char *maze_difficulty_name(MazeDifficulty p_difficulty);
// Opciones del nivel (la semilla se deja en 1; se asigna al generar).
MazeGrid::Options maze_preset(MazeDifficulty p_difficulty);

} // namespace lv::core
