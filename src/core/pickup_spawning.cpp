#include "core/pickup_spawning.h"

#include <algorithm>
#include <cmath>

namespace lv::core {

std::optional<Cell> pick_spawn_cell(const MazeGrid &p_grid, const std::vector<Cell> &p_occupied,
		const std::vector<Cell> &p_bases, const std::vector<SpawnObserver> &p_observers,
		const SpawnCellOptions &p_options, Rng &r_rng) {
	// Versión con lista (tests y pocos objetos): se adapta a la versión con índice.
	const HasPickupAt has_pickup = [&p_occupied](const Cell &p_cell) {
		for (const Cell &c : p_occupied) {
			if (c == p_cell) {
				return true;
			}
		}
		return false;
	};
	return pick_spawn_cell_indexed(p_grid, has_pickup, p_bases, p_observers, p_options, r_rng);
}

std::optional<Cell> pick_spawn_cell_indexed(const MazeGrid &p_grid, const HasPickupAt &p_has_pickup_at,
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
		// Separación: solo la ventana alrededor de la candidata (distancia de Chebyshev < min_spacing).
		const int reach = p_options.min_spacing - 1;
		for (int dy = -reach; dy <= reach && !rejected; dy++) {
			for (int dx = -reach; dx <= reach && !rejected; dx++) {
				rejected = p_has_pickup_at({ cell.x + dx, cell.y + dy });
			}
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

int category_target(int p_budget, float p_share, int p_min_count, int p_max_count) {
	int target = static_cast<int>(std::lround(std::max(0, p_budget) * std::max(0.0f, p_share)));
	target = std::max(target, std::max(0, p_min_count));
	if (p_max_count > 0) {
		target = std::min(target, p_max_count);
	}
	return target;
}

float refill_delay(int p_target, float p_refill_time) {
	if (p_target <= 0 || p_refill_time <= 0.0f) {
		return 0.0f;
	}
	return p_refill_time / static_cast<float>(p_target);
}

void ExpiryQueue::schedule(uint64_t p_id, double p_now, double p_lifetime, double p_warn_time) {
	if (p_lifetime <= 0.0) {
		return;
	}
	const double expire_at = p_now + p_lifetime;
	if (p_warn_time > 0.0) {
		queue_.push({ std::max(p_now, expire_at - p_warn_time), p_id, Stage::Warn });
	}
	queue_.push({ expire_at, p_id, Stage::Expire });
}

std::vector<ExpiryQueue::Event> ExpiryQueue::poll(double p_now) {
	std::vector<Event> events;
	while (!queue_.empty() && queue_.top().time <= p_now) {
		events.push_back({ queue_.top().id, queue_.top().stage });
		queue_.pop();
	}
	return events;
}

} // namespace lv::core
