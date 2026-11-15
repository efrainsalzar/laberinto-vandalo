#include "core/maze_algorithms.h"

#include <numeric>
#include <queue>

namespace lv::core {

const char *maze_algorithm_name(MazeAlgorithm p_algorithm) {
	switch (p_algorithm) {
		case MazeAlgorithm::RecursiveBacktracker:
			return "Backtracking";
		case MazeAlgorithm::GrowingTree:
			return "Growing Tree";
		case MazeAlgorithm::Prim:
			return "Prim";
		case MazeAlgorithm::Kruskal:
			return "Kruskal";
		case MazeAlgorithm::Wilson:
			return "Wilson";
		case MazeAlgorithm::HuntAndKill:
			return "Hunt-and-Kill";
		default:
			return "?";
	}
}

// --- PassageGrid --------------------------------------------------------------------------

PassageGrid::PassageGrid(int p_width, int p_height) :
		width_(p_width < 1 ? 1 : p_width), height_(p_height < 1 ? 1 : p_height) {
	east_.assign(static_cast<size_t>(width_) * height_, 0);
	south_.assign(static_cast<size_t>(width_) * height_, 0);
}

bool PassageGrid::in_bounds(const Cell &p_cell) const {
	return p_cell.x >= 0 && p_cell.y >= 0 && p_cell.x < width_ && p_cell.y < height_;
}

int PassageGrid::neighbors(int p_index, int r_out[4]) const {
	const Cell c = cell(p_index);
	int count = 0;
	if (c.x + 1 < width_) {
		r_out[count++] = p_index + 1;
	}
	if (c.x > 0) {
		r_out[count++] = p_index - 1;
	}
	if (c.y + 1 < height_) {
		r_out[count++] = p_index + width_;
	}
	if (c.y > 0) {
		r_out[count++] = p_index - width_;
	}
	return count;
}

bool PassageGrid::is_open(int p_a, int p_b) const {
	if (p_a > p_b) {
		std::swap(p_a, p_b);
	}
	if (p_b == p_a + 1 && cell(p_a).y == cell(p_b).y) {
		return east_[p_a] != 0;
	}
	if (p_b == p_a + width_) {
		return south_[p_a] != 0;
	}
	return false;
}

void PassageGrid::carve(int p_a, int p_b) {
	if (p_a > p_b) {
		std::swap(p_a, p_b);
	}
	if (p_b == p_a + 1 && cell(p_a).y == cell(p_b).y) {
		east_[p_a] = 1;
	} else if (p_b == p_a + width_) {
		south_[p_a] = 1;
	}
}

int PassageGrid::degree(int p_index) const {
	int n[4];
	const int count = neighbors(p_index, n);
	int open = 0;
	for (int i = 0; i < count; i++) {
		open += is_open(p_index, n[i]) ? 1 : 0;
	}
	return open;
}

int PassageGrid::passage_count() const {
	int total = 0;
	for (size_t i = 0; i < east_.size(); i++) {
		total += east_[i] + south_[i];
	}
	return total;
}

bool PassageGrid::is_connected() const {
	std::vector<uint8_t> seen(static_cast<size_t>(cell_count()), 0);
	std::queue<int> open;
	open.push(0);
	seen[0] = 1;
	int reached = 1;
	while (!open.empty()) {
		const int c = open.front();
		open.pop();
		int n[4];
		const int count = neighbors(c, n);
		for (int i = 0; i < count; i++) {
			if (!seen[n[i]] && is_open(c, n[i])) {
				seen[n[i]] = 1;
				reached++;
				open.push(n[i]);
			}
		}
	}
	return reached == cell_count();
}

// --- Algoritmos ---------------------------------------------------------------------------

namespace {

int random_unvisited_neighbor(const PassageGrid &p_grid, int p_cell, const std::vector<uint8_t> &p_visited, Rng &r_rng) {
	int n[4];
	int options[4];
	int option_count = 0;
	const int count = p_grid.neighbors(p_cell, n);
	for (int i = 0; i < count; i++) {
		if (!p_visited[n[i]]) {
			options[option_count++] = n[i];
		}
	}
	return option_count == 0 ? -1 : options[r_rng.range(option_count)];
}

// Growing Tree: lista de celdas activas; se elige la más nueva (con prob. `bias`) o una al azar.
// bias = 1 es exactamente el backtracking recursivo (versión iterativa).
void growing_tree(PassageGrid &r_grid, Rng &r_rng, float p_bias) {
	std::vector<uint8_t> visited(static_cast<size_t>(r_grid.cell_count()), 0);
	std::vector<int> active;
	const int start = r_rng.range(r_grid.cell_count());
	visited[start] = 1;
	active.push_back(start);
	while (!active.empty()) {
		const int idx = (p_bias >= 1.0f || r_rng.chance(p_bias)) ? static_cast<int>(active.size()) - 1 : r_rng.range(static_cast<int>(active.size()));
		const int current = active[idx];
		const int next = random_unvisited_neighbor(r_grid, current, visited, r_rng);
		if (next < 0) {
			active.erase(active.begin() + idx);
			continue;
		}
		r_grid.carve(current, next);
		visited[next] = 1;
		active.push_back(next);
	}
}

// Prim (aleatorizado): crece desde una celda eligiendo al azar una celda de la "frontera"
// y uniéndola a un vecino ya visitado.
void prim(PassageGrid &r_grid, Rng &r_rng) {
	const int total = r_grid.cell_count();
	std::vector<uint8_t> visited(static_cast<size_t>(total), 0);
	std::vector<uint8_t> in_frontier(static_cast<size_t>(total), 0);
	std::vector<int> frontier;
	auto visit = [&](int p_cell) {
		visited[p_cell] = 1;
		int n[4];
		const int count = r_grid.neighbors(p_cell, n);
		for (int i = 0; i < count; i++) {
			if (!visited[n[i]] && !in_frontier[n[i]]) {
				in_frontier[n[i]] = 1;
				frontier.push_back(n[i]);
			}
		}
	};
	visit(r_rng.range(total));
	while (!frontier.empty()) {
		const int pick = r_rng.range(static_cast<int>(frontier.size()));
		const int cell = frontier[pick];
		frontier[pick] = frontier.back();
		frontier.pop_back();
		int n[4];
		int inside[4];
		int inside_count = 0;
		const int count = r_grid.neighbors(cell, n);
		for (int i = 0; i < count; i++) {
			if (visited[n[i]]) {
				inside[inside_count++] = n[i];
			}
		}
		r_grid.carve(cell, inside[r_rng.range(inside_count)]);
		visit(cell);
	}
}

// Kruskal: todas las paredes en orden aleatorio; se abre si separa dos grupos distintos (union-find).
void kruskal(PassageGrid &r_grid, Rng &r_rng) {
	const int total = r_grid.cell_count();
	std::vector<int> parent(static_cast<size_t>(total));
	std::iota(parent.begin(), parent.end(), 0);
	auto find = [&](int p_x) {
		while (parent[p_x] != p_x) {
			parent[p_x] = parent[parent[p_x]];
			p_x = parent[p_x];
		}
		return p_x;
	};
	struct Edge {
		int a;
		int b;
	};
	std::vector<Edge> edges;
	for (int i = 0; i < total; i++) {
		const Cell c = r_grid.cell(i);
		if (c.x + 1 < r_grid.width()) {
			edges.push_back({ i, i + 1 });
		}
		if (c.y + 1 < r_grid.height()) {
			edges.push_back({ i, i + r_grid.width() });
		}
	}
	r_rng.shuffle(edges);
	for (const Edge &e : edges) {
		const int ra = find(e.a);
		const int rb = find(e.b);
		if (ra != rb) {
			parent[ra] = rb;
			r_grid.carve(e.a, e.b);
		}
	}
}

// Wilson: caminatas aleatorias con borrado de bucles hasta tocar el árbol. Genera un árbol
// uniforme: todos los laberintos posibles tienen la misma probabilidad (sin sesgo).
void wilson(PassageGrid &r_grid, Rng &r_rng) {
	const int total = r_grid.cell_count();
	std::vector<uint8_t> in_tree(static_cast<size_t>(total), 0);
	std::vector<int> next(static_cast<size_t>(total), -1);
	in_tree[r_rng.range(total)] = 1;
	std::vector<int> order(static_cast<size_t>(total));
	std::iota(order.begin(), order.end(), 0);
	r_rng.shuffle(order);
	for (int start : order) {
		if (in_tree[start]) {
			continue;
		}
		// Caminata: al pisar de nuevo una celda se sobrescribe su dirección (borra el bucle).
		int c = start;
		while (!in_tree[c]) {
			int n[4];
			const int count = r_grid.neighbors(c, n);
			next[c] = n[r_rng.range(count)];
			c = next[c];
		}
		c = start;
		while (!in_tree[c]) {
			r_grid.carve(c, next[c]);
			in_tree[c] = 1;
			c = next[c];
		}
	}
}

// Hunt-and-Kill: camina al azar mientras pueda; si se atasca, "caza" la primera celda sin visitar
// junto a una visitada, la une y sigue desde ahí.
void hunt_and_kill(PassageGrid &r_grid, Rng &r_rng) {
	const int total = r_grid.cell_count();
	std::vector<uint8_t> visited(static_cast<size_t>(total), 0);
	int current = r_rng.range(total);
	visited[current] = 1;
	while (current >= 0) {
		const int next = random_unvisited_neighbor(r_grid, current, visited, r_rng);
		if (next >= 0) {
			r_grid.carve(current, next);
			visited[next] = 1;
			current = next;
			continue;
		}
		current = -1;
		for (int i = 0; i < total && current < 0; i++) {
			if (visited[i]) {
				continue;
			}
			int n[4];
			int inside[4];
			int inside_count = 0;
			const int count = r_grid.neighbors(i, n);
			for (int k = 0; k < count; k++) {
				if (visited[n[k]]) {
					inside[inside_count++] = n[k];
				}
			}
			if (inside_count > 0) {
				r_grid.carve(i, inside[r_rng.range(inside_count)]);
				visited[i] = 1;
				current = i;
			}
		}
	}
}

} // namespace

void carve_maze(MazeAlgorithm p_algorithm, PassageGrid &r_grid, Rng &r_rng, float p_growing_tree_bias) {
	switch (p_algorithm) {
		case MazeAlgorithm::GrowingTree:
			growing_tree(r_grid, r_rng, p_growing_tree_bias);
			break;
		case MazeAlgorithm::Prim:
			prim(r_grid, r_rng);
			break;
		case MazeAlgorithm::Kruskal:
			kruskal(r_grid, r_rng);
			break;
		case MazeAlgorithm::Wilson:
			wilson(r_grid, r_rng);
			break;
		case MazeAlgorithm::HuntAndKill:
			hunt_and_kill(r_grid, r_rng);
			break;
		case MazeAlgorithm::RecursiveBacktracker:
		default:
			growing_tree(r_grid, r_rng, 1.0f);
			break;
	}
}

void add_loops(PassageGrid &r_grid, float p_chance, Rng &r_rng) {
	if (p_chance <= 0.0f) {
		return;
	}
	for (int i = 0; i < r_grid.cell_count(); i++) {
		const Cell c = r_grid.cell(i);
		if (c.x + 1 < r_grid.width() && !r_grid.is_open(i, i + 1) && r_rng.chance(p_chance)) {
			r_grid.carve(i, i + 1);
		}
		if (c.y + 1 < r_grid.height() && !r_grid.is_open(i, i + r_grid.width()) && r_rng.chance(p_chance)) {
			r_grid.carve(i, i + r_grid.width());
		}
	}
}

void braid(PassageGrid &r_grid, float p_chance, Rng &r_rng) {
	if (p_chance <= 0.0f) {
		return;
	}
	std::vector<int> order(static_cast<size_t>(r_grid.cell_count()));
	std::iota(order.begin(), order.end(), 0);
	r_rng.shuffle(order);
	for (int cell : order) {
		if (r_grid.degree(cell) != 1 || !r_rng.chance(p_chance)) {
			continue;
		}
		int n[4];
		int closed[4];
		int dead[4];
		int closed_count = 0;
		int dead_count = 0;
		const int count = r_grid.neighbors(cell, n);
		for (int i = 0; i < count; i++) {
			if (!r_grid.is_open(cell, n[i])) {
				closed[closed_count++] = n[i];
				if (r_grid.degree(n[i]) == 1) {
					dead[dead_count++] = n[i];
				}
			}
		}
		if (closed_count == 0) {
			continue;
		}
		// Unir dos callejones de una vez elimina ambos con un solo pasaje.
		const int target = dead_count > 0 ? dead[r_rng.range(dead_count)] : closed[r_rng.range(closed_count)];
		r_grid.carve(cell, target);
	}
}

} // namespace lv::core
