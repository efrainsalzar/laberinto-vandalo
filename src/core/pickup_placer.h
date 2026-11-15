#pragma once

#include "core/cell.h"
#include "core/maze_grid.h"

#include <cstdint>
#include <vector>

namespace lv::core {

// Dónde aparece cada objeto recolectable y de qué tipo es (índice en la lista de pesos).
struct PickupPlacement {
	Cell cell;
	int kind = 0;
};

struct PickupPlacementOptions {
	int count = 30;
	int min_spacing = 3; // distancia mínima entre objetos (casillas, Chebyshev)
	int base_clearance = 6; // ningún objeto a esta distancia o menos de una base
	bool avoid_vault = true; // la bóveda es del trofeo
	uint32_t seed = 1;
};

// Elige casillas de suelo para los objetos y el tipo de cada uno según su peso (rareza).
// Puede devolver menos de `count` si no hay espacio suficiente con esas restricciones.
// Determinista: misma semilla y opciones => mismo resultado en todas las plataformas.
// Ver docs/ARQUITECTURA.md §8.
std::vector<PickupPlacement> place_pickups(const MazeGrid &p_grid, const std::vector<float> &p_kind_weights,
		const std::vector<Cell> &p_bases, const PickupPlacementOptions &p_options);

} // namespace lv::core
