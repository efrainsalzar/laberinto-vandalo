#include "core/pickup_placer.h"

#include "core/random.h"

namespace lv::core {

std::vector<PickupPlacement> place_pickups(const MazeGrid &p_grid, const std::vector<float> &p_kind_weights,
		const std::vector<Cell> &p_bases, const PickupPlacementOptions &p_options) {
	std::vector<PickupPlacement> result;
	if (p_options.count <= 0) {
		return result;
	}
	Rng rng(p_options.seed);

	// 1) Candidatas: suelo, fuera de la bóveda y lejos de las bases.
	std::vector<Cell> candidates;
	for (const Cell &cell : p_grid.floor_cells()) {
		if (p_options.avoid_vault && p_grid.is_in_vault(cell)) {
			continue;
		}
		bool near_base = false;
		for (const Cell &base : p_bases) {
			if (chebyshev_distance(cell, base) <= p_options.base_clearance) {
				near_base = true;
				break;
			}
		}
		if (!near_base) {
			candidates.push_back(cell);
		}
	}

	// 2) Orden aleatorio y se toman las que respetan la separación mínima.
	rng.shuffle(candidates);
	for (const Cell &cell : candidates) {
		if (static_cast<int>(result.size()) >= p_options.count) {
			break;
		}
		bool too_close = false;
		for (const PickupPlacement &placed : result) {
			if (chebyshev_distance(cell, placed.cell) < p_options.min_spacing) {
				too_close = true;
				break;
			}
		}
		if (too_close) {
			continue;
		}
		const int kind = rng.weighted(p_kind_weights);
		result.push_back({ cell, kind < 0 ? 0 : kind });
	}
	return result;
}

} // namespace lv::core
