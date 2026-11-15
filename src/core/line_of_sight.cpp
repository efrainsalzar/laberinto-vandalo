#include "core/line_of_sight.h"

#include <cmath>
#include <limits>

namespace lv::core {

bool has_line_of_sight(const MazeGrid &p_grid, const GridPoint &p_from, const GridPoint &p_to) {
	int x = static_cast<int>(std::floor(p_from.x));
	int y = static_cast<int>(std::floor(p_from.y));
	const int end_x = static_cast<int>(std::floor(p_to.x));
	const int end_y = static_cast<int>(std::floor(p_to.y));

	if (p_grid.is_wall({ x, y }) || p_grid.is_wall({ end_x, end_y })) {
		return false;
	}

	const float dx = p_to.x - p_from.x;
	const float dy = p_to.y - p_from.y;
	const int step_x = dx > 0 ? 1 : (dx < 0 ? -1 : 0);
	const int step_y = dy > 0 ? 1 : (dy < 0 ? -1 : 0);
	constexpr float INF = std::numeric_limits<float>::infinity();

	// Distancia (en fracción del segmento) para cruzar una casilla entera en cada eje,
	// y hasta el primer borde de casilla en cada eje.
	const float delta_x = step_x != 0 ? std::abs(1.0f / dx) : INF;
	const float delta_y = step_y != 0 ? std::abs(1.0f / dy) : INF;
	float next_x = step_x > 0 ? (x + 1 - p_from.x) * delta_x : (step_x < 0 ? (p_from.x - x) * delta_x : INF);
	float next_y = step_y > 0 ? (y + 1 - p_from.y) * delta_y : (step_y < 0 ? (p_from.y - y) * delta_y : INF);

	while (x != end_x || y != end_y) {
		if (next_x < next_y) {
			x += step_x;
			next_x += delta_x;
		} else if (next_y < next_x) {
			y += step_y;
			next_y += delta_y;
		} else {
			// Pasa justo por una esquina: no se ve a través de dos paredes en diagonal.
			if (p_grid.is_wall({ x + step_x, y }) && p_grid.is_wall({ x, y + step_y })) {
				return false;
			}
			x += step_x;
			y += step_y;
			next_x += delta_x;
			next_y += delta_y;
		}
		if (p_grid.is_wall({ x, y })) {
			return false;
		}
		if (next_x > 1.0f && next_y > 1.0f) {
			break; // ya se llegó al final del segmento
		}
	}
	return true;
}

bool can_see(const MazeGrid &p_grid, const GridPoint &p_from, const GridPoint &p_to, float p_radius) {
	const float dx = p_to.x - p_from.x;
	const float dy = p_to.y - p_from.y;
	if (dx * dx + dy * dy > p_radius * p_radius) {
		return false;
	}
	return has_line_of_sight(p_grid, p_from, p_to);
}

} // namespace lv::core
