#include "core/pickup_spawning.h"

namespace lv::core {

std::optional<Cell> pick_spawn_cell(const MazeGrid &p_grid, const std::vector<Cell> &p_occupied,
		const std::vector<Cell> &p_bases, const std::vector<SpawnObserver> &p_observers,
		const SpawnCellOptions &p_options, Rng &r_rng) {
	if (p_grid.width() <= 2 || p_grid.height() <= 2) {
		return std::nullopt;
	}
	for (int attempt = 0; attempt < p_options.max_attempts; attempt++) {
		// Muestreo aleatorio dentro del borde: más barato que recorrer todo el suelo cada vez.
		const Cell cell{ 1 + r_rng.range(p_grid.width() - 2), 1 + r_rng.range(p_grid.height() - 2) };
		if (p_grid.is_wall(cell) || (p_options.avoid_vault && p_grid.is_in_vault(cell))) {
			continue;
		}
		bool rejected = false;
		for (const Cell &base : p_bases) {
			if (chebyshev_distance(cell, base) <= p_options.base_clearance) {
				rejected = true;
				break;
			}
		}
		for (size_t i = 0; i < p_occupied.size() && !rejected; i++) {
			rejected = chebyshev_distance(cell, p_occupied[i]) < p_options.min_spacing;
		}
		const GridPoint center{ cell.x + 0.5f, cell.y + 0.5f };
		for (size_t i = 0; i < p_observers.size() && !rejected; i++) {
			const SpawnObserver &observer = p_observers[i];
			rejected = can_see(p_grid, observer.position, center, observer.vision_radius + p_options.observer_margin);
		}
		if (!rejected) {
			return cell;
		}
	}
	return std::nullopt;
}

int KeepDensityRule::update(int p_active_count, double p_delta) {
	if (p_active_count >= target_count) {
		timer_ = 0.0; // lleno: el reloj empieza a contar cuando falte uno
		return 0;
	}
	timer_ += p_delta;
	if (respawn_delay <= 0.0f) {
		timer_ = 0.0;
		return target_count - p_active_count;
	}
	int spawns = 0;
	while (timer_ >= respawn_delay && p_active_count + spawns < target_count) {
		timer_ -= respawn_delay;
		spawns++;
	}
	return spawns;
}

} // namespace lv::core
