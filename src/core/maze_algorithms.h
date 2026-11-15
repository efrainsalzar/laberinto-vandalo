#pragma once

#include "core/cell.h"
#include "core/random.h"

#include <cstdint>
#include <vector>

namespace lv::core {

// Algoritmos clásicos de generación de laberintos. Todos producen un laberinto "perfecto"
// (un árbol: exactamente un camino entre dos celdas); los bucles y la eliminación de
// callejones se aplican después. Cada uno tiene su "personalidad":
//
//   RecursiveBacktracker  pasillos muy largos y sinuosos, pocos callejones          (~10 %)
//   HuntAndKill           parecido al anterior, algo más ramificado                  (~10 %)
//   GrowingTree           mezcla configurable entre backtracker (1.0) y Prim (0.0)
//   Kruskal               uniforme, muchos cruces y callejones cortos                (~30 %)
//   Wilson                uniforme sin sesgo (todos los laberintos igual de probables) (~30 %)
//   Prim                  muy ramificado, muchísimos callejones cortos, confunde     (~35 %)
//
// (Entre paréntesis: proporción aproximada de celdas que son callejones sin salida.)
// Patrón Strategy: el algoritmo se elige por dato (enum) y se ejecuta por la misma interfaz.
// Ver docs/ARQUITECTURA.md §12.
enum class MazeAlgorithm : int {
	RecursiveBacktracker = 0,
	GrowingTree = 1,
	Prim = 2,
	Kruskal = 3,
	Wilson = 4,
	HuntAndKill = 5,
	Count
};

const char *maze_algorithm_name(MazeAlgorithm p_algorithm);

// Topología del laberinto: celdas en una cuadrícula y pasajes abiertos entre celdas vecinas.
// No sabe de casillas, pared ni ancho de pasillo (eso lo resuelve MazeGrid al "dibujarla").
// Es el paso hacia mapas con otras formas (DISENO_MAPA §2).
class PassageGrid {
public:
	PassageGrid(int p_width, int p_height);

	int width() const { return width_; }
	int height() const { return height_; }
	int cell_count() const { return width_ * height_; }
	int index(const Cell &p_cell) const { return p_cell.y * width_ + p_cell.x; }
	Cell cell(int p_index) const { return { p_index % width_, p_index / width_ }; }
	bool in_bounds(const Cell &p_cell) const;

	// Vecinos dentro de la cuadrícula (2 a 4), en orden fijo.
	int neighbors(int p_index, int r_out[4]) const;
	bool is_open(int p_a, int p_b) const;
	void carve(int p_a, int p_b);
	void close(int p_a, int p_b);
	bool are_adjacent(int p_a, int p_b) const;
	int degree(int p_index) const; // pasajes abiertos de una celda
	int passage_count() const;

	bool is_connected() const;

private:
	int width_;
	int height_;
	std::vector<uint8_t> east_; // pasaje abierto hacia (x+1, y)
	std::vector<uint8_t> south_; // pasaje abierto hacia (x, y+1)
};

// Genera un laberinto perfecto con el algoritmo pedido. `p_growing_tree_bias` solo lo usa GrowingTree:
// probabilidad de continuar desde la celda más nueva (1 = backtracker, 0 = Prim).
void carve_maze(MazeAlgorithm p_algorithm, PassageGrid &r_grid, Rng &r_rng, float p_growing_tree_bias = 0.5f);

// Abre pasajes extra al azar entre celdas vecinas (crea bucles: rutas alternativas).
void add_loops(PassageGrid &r_grid, float p_chance, Rng &r_rng);

// "Braid": cada callejón sin salida se abre a un vecino con probabilidad p_chance
// (prefiriendo otro callejón). 1.0 elimina todos los callejones.
void braid(PassageGrid &r_grid, float p_chance, Rng &r_rng);

} // namespace lv::core
