#include "core/nav_path.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <queue>

namespace lv::core {

std::vector<int> find_path(const PassageGrid &p_grid, int p_from, int p_to, const std::vector<float> *p_extra_cost,
		int p_max_visits) {
	const int count = p_grid.cell_count();
	if (p_from < 0 || p_to < 0 || p_from >= count || p_to >= count) {
		return {};
	}
	if (p_from == p_to) {
		return { p_from };
	}
	const Cell goal = p_grid.cell(p_to);
	auto heuristic = [&](int p_index) {
		const Cell c = p_grid.cell(p_index);
		return static_cast<float>(std::abs(c.x - goal.x) + std::abs(c.y - goal.y));
	};
	const bool use_costs = p_extra_cost != nullptr && static_cast<int>(p_extra_cost->size()) == count;

	std::vector<float> g(static_cast<size_t>(count), -1.0f);
	std::vector<int> parent(static_cast<size_t>(count), -1);
	using Node = std::pair<float, int>; // (f, celda)
	std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
	g[p_from] = 0.0f;
	open.push({ heuristic(p_from), p_from });
	int visits = 0;
	while (!open.empty()) {
		const auto [f, current] = open.top();
		open.pop();
		if (current == p_to) {
			break;
		}
		if (f - heuristic(current) > g[current] + 1e-4f) {
			continue; // entrada vieja de la cola
		}
		if (++visits > p_max_visits) {
			return {};
		}
		int neighbors[4];
		const int n = p_grid.neighbors(current, neighbors);
		for (int k = 0; k < n; k++) {
			const int next = neighbors[k];
			if (!p_grid.is_open(current, next)) {
				continue;
			}
			const float step = 1.0f + (use_costs ? std::max(0.0f, (*p_extra_cost)[next]) : 0.0f);
			const float cost = g[current] + step;
			if (g[next] < 0.0f || cost < g[next]) {
				g[next] = cost;
				parent[next] = current;
				open.push({ cost + heuristic(next), next });
			}
		}
	}
	if (g[p_to] < 0.0f) {
		return {};
	}
	std::vector<int> path;
	for (int c = p_to; c != -1; c = parent[c]) {
		path.push_back(c);
	}
	std::reverse(path.begin(), path.end());
	return path;
}

int choose_next_cell(const PassageGrid &p_grid, int p_current, int p_planned_next, int p_previous,
		float p_mistake_chance, Rng &r_rng) {
	if (p_mistake_chance <= 0.0f || p_grid.degree(p_current) < 3 || !r_rng.chance(p_mistake_chance)) {
		return p_planned_next;
	}
	int neighbors[4];
	const int n = p_grid.neighbors(p_current, neighbors);
	std::vector<int> options;
	for (int k = 0; k < n; k++) {
		const int c = neighbors[k];
		if (c != p_planned_next && c != p_previous && p_grid.is_open(p_current, c)) {
			options.push_back(c);
		}
	}
	return options.empty() ? p_planned_next : options[r_rng.range(static_cast<int>(options.size()))];
}

} // namespace lv::core
