#pragma once

#include "core/cell.h"
#include "core/line_of_sight.h"
#include "core/maze_grid.h"
#include "core/random.h"

#include <functional>
#include <optional>
#include <vector>

namespace lv::core {

// Alguien que mira el laberinto (un participante): no deben aparecer objetos frente a él.
struct SpawnObserver {
	GridPoint position; // en casillas (con decimales)
	float vision_radius = 5.0f; // casillas
};

struct SpawnCellOptions {
	int min_spacing = 4; // distancia mínima a otros objetos (casillas, Chebyshev)
	int base_clearance = 6; // distancia mínima a las bases
	float observer_margin = 2.0f; // casillas extra más allá del radio de visión
	bool avoid_vault = true;
	int max_attempts = 200; // intentos aleatorios antes de rendirse
};

// Elige una casilla para que aparezca un objeto nuevo:
// suelo, fuera de la bóveda, lejos de bases y de otros objetos, y donde NINGÚN observador
// la esté viendo (fuera de su radio + margen, o tapada por paredes).
// Devuelve nullopt si no encuentra lugar (se reintenta más tarde).
std::optional<Cell> pick_spawn_cell(const MazeGrid &p_grid, const std::vector<Cell> &p_occupied,
		const std::vector<Cell> &p_bases, const std::vector<SpawnObserver> &p_observers,
		const SpawnCellOptions &p_options, Rng &r_rng);

// Igual, pero "¿hay un objeto en esta casilla?" se pregunta a un índice (p. ej. espacial): la separación
// mínima se comprueba solo en la ventana alrededor de cada candidata, no contra todos los objetos.
// Costo: O(intentos × min_spacing²) sin importar cuántos objetos haya. (RENDIMIENTO.md, regla 7)
using HasPickupAt = std::function<bool(const Cell &)>;
std::optional<Cell> pick_spawn_cell_indexed(const MazeGrid &p_grid, const HasPickupAt &p_has_pickup_at,
		const std::vector<Cell> &p_bases, const std::vector<SpawnObserver> &p_observers,
		const SpawnCellOptions &p_options, Rng &r_rng);

// Regla "mantener la densidad": si hay menos objetos que `target_count`, aparece uno nuevo
// cada `respawn_delay` segundos. Es la regla por defecto; el Director de IA (fase 5) podrá
// usar otra (patrón Strategy en la capa de Godot: PickupSpawnPolicy).
class KeepDensityRule {
public:
	int target_count = 30;
	float respawn_delay = 3.0f; // segundos entre apariciones mientras falten objetos

	// Cuántos objetos deben aparecer en este paso.
	int update(int p_active_count, double p_delta);

private:
	double timer_ = 0.0;
};

} // namespace lv::core
