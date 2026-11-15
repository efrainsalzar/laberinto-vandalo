#pragma once

#include "core/maze_grid.h"
#include "core/random.h"

#include <vector>

namespace lv::core {

// Un cambio del laberinto vivo: abrir o cerrar el pasaje entre dos celdas vecinas.
struct PassageChange {
	int a = -1;
	int b = -1;
	bool open = false;
};

// Planifica un "movimiento" del laberinto: `p_pairs` aperturas + `p_pairs` cierres (la cantidad de pasajes
// se mantiene: el laberinto cambia de forma sin volverse más fácil ni más difícil).
//
// Garantías (con los cambios aplicados en orden, aperturas primero):
//   - nunca se desconecta el laberinto (cada cierre se verifica tras los anteriores);
//   - nunca se toca la bóveda, las bases ni el borde (MazeGrid::is_protected_passage).
// Si se cancela un cierre (por ejemplo, alguien está parado ahí), las garantías se mantienen:
// cancelar un cierre solo deja más pasajes abiertos.
// Ver docs/ARQUITECTURA.md §15.
std::vector<PassageChange> plan_maze_shift(const MazeGrid &p_grid, int p_pairs, Rng &r_rng);

} // namespace lv::core
