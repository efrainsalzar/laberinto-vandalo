#include "core/maze_grid.h"

#include <algorithm>
#include <array>
#include <queue>

namespace lv::core {

namespace {
const std::array<Cell, 4> DIRECTIONS = { Cell{ 1, 0 }, Cell{ -1, 0 }, Cell{ 0, 1 }, Cell{ 0, -1 } };
} // namespace

void MazeGrid::generate(const Options &p_options) {
	const int cells_w = std::max(2, p_options.cells_w);
	const int cells_h = std::max(2, p_options.cells_h);
	width_ = cells_w * 2 + 1;
	height_ = cells_h * 2 + 1;
	vault_radius_ = std::max(0, p_options.vault_radius);
	walls_.assign(static_cast<size_t>(width_) * height_, 1);

	corridor_width_ = std::max(1, p_options.corridor_width);

	Rng rng(p_options.seed);
	carve_perfect_maze(cells_w, cells_h, rng);
	open_loops(p_options.loop_chance, rng);
	widen_corridors(corridor_width_);
	open_vault();
}

// Paso 3: escala la cuadrícula compacta. Cada celda (coordenada impar) pasa a medir
// corridor_width casillas y cada pared (coordenada par) sigue midiendo 1.
void MazeGrid::widen_corridors(int p_corridor_width) {
	if (p_corridor_width == 1) {
		return;
	}
	const int compact_w = width_;
	const int compact_h = height_;
	const std::vector<uint8_t> compact = walls_;
	const int step = p_corridor_width + 1;
	auto start = [step](int p_i) { return (p_i / 2) * step + (p_i % 2); };
	auto span = [p_corridor_width](int p_i) { return (p_i % 2) ? p_corridor_width : 1; };

	width_ = (compact_w / 2) * step + 1;
	height_ = (compact_h / 2) * step + 1;
	walls_.assign(static_cast<size_t>(width_) * height_, 1);
	for (int sy = 0; sy < compact_h; sy++) {
		for (int sx = 0; sx < compact_w; sx++) {
			if (compact[sy * compact_w + sx] != 0) {
				continue;
			}
			for (int y = start(sy); y < start(sy) + span(sy); y++) {
				for (int x = start(sx); x < start(sx) + span(sx); x++) {
					set_wall({ x, y }, false);
				}
			}
		}
	}
}

int MazeGrid::compact_to_final(int p_compact) const {
	const int step = corridor_width_ + 1;
	return (p_compact / 2) * step + 1 + corridor_width_ / 2;
}

// Paso 1: backtracking iterativo. Recorre las celdas al azar abriendo paredes;
// el resultado tiene exactamente un camino entre cualquier par de celdas.
void MazeGrid::carve_perfect_maze(int p_cells_w, int p_cells_h, Rng &r_rng) {
	std::vector<uint8_t> visited(static_cast<size_t>(p_cells_w) * p_cells_h, 0);
	std::vector<Cell> stack;
	stack.push_back({ 0, 0 });
	visited[0] = 1;
	set_wall({ 1, 1 }, false);

	while (!stack.empty()) {
		const Cell current = stack.back();
		std::array<Cell, 4> options{};
		int option_count = 0;
		for (const Cell &d : DIRECTIONS) {
			const Cell next{ current.x + d.x, current.y + d.y };
			if (next.x >= 0 && next.y >= 0 && next.x < p_cells_w && next.y < p_cells_h && !visited[next.y * p_cells_w + next.x]) {
				options[option_count++] = next;
			}
		}
		if (option_count == 0) {
			stack.pop_back();
			continue;
		}
		const Cell next = options[r_rng.range(option_count)];
		visited[next.y * p_cells_w + next.x] = 1;
		set_wall({ next.x * 2 + 1, next.y * 2 + 1 }, false); // celda destino
		set_wall({ current.x + next.x + 1, current.y + next.y + 1 }, false); // pared entre ambas
		stack.push_back(next);
	}
}

// Paso 2: abre algunas paredes que separan dos pasillos, creando rutas alternativas.
void MazeGrid::open_loops(float p_chance, Rng &r_rng) {
	if (p_chance <= 0.0f) {
		return;
	}

	for (int y = 1; y < height_ - 1; y++) {
		for (int x = 1; x < width_ - 1; x++) {
			if (!is_wall({ x, y })) {
				continue;
			}
			const bool between_horizontal = !is_wall({ x - 1, y }) && !is_wall({ x + 1, y }) && is_wall({ x, y - 1 }) && is_wall({ x, y + 1 });
			const bool between_vertical = !is_wall({ x, y - 1 }) && !is_wall({ x, y + 1 }) && is_wall({ x - 1, y }) && is_wall({ x + 1, y });
			if ((between_horizontal || between_vertical) && r_rng.chance(p_chance)) {
				set_wall({ x, y }, false);
			}
		}
	}
}

// Paso 4: sala abierta en el centro (la bóveda del trofeo). Nunca toca el borde exterior.
void MazeGrid::open_vault() {
	const Cell c = center();
	for (int y = c.y - vault_radius_; y <= c.y + vault_radius_; y++) {
		for (int x = c.x - vault_radius_; x <= c.x + vault_radius_; x++) {
			if (x > 0 && y > 0 && x < width_ - 1 && y < height_ - 1) {
				set_wall({ x, y }, false);
			}
		}
	}
}

bool MazeGrid::in_bounds(const Cell &p_cell) const {
	return p_cell.x >= 0 && p_cell.y >= 0 && p_cell.x < width_ && p_cell.y < height_;
}

bool MazeGrid::is_wall(const Cell &p_cell) const {
	return !in_bounds(p_cell) || walls_[index(p_cell)] != 0;
}

void MazeGrid::set_wall(const Cell &p_cell, bool p_wall) {
	if (in_bounds(p_cell)) {
		walls_[index(p_cell)] = p_wall ? 1 : 0;
	}
}

std::vector<Cell> MazeGrid::floor_cells() const {
	std::vector<Cell> result;
	for (int y = 0; y < height_; y++) {
		for (int x = 0; x < width_; x++) {
			if (!is_wall({ x, y })) {
				result.push_back({ x, y });
			}
		}
	}
	return result;
}

Cell MazeGrid::center() const {
	return { width_ / 2, height_ / 2 };
}

bool MazeGrid::is_in_vault(const Cell &p_cell) const {
	return vault_radius_ > 0 && chebyshev_distance(p_cell, center()) <= vault_radius_;
}

std::vector<Cell> MazeGrid::base_cells(int p_count) const {
	// Se eligen celdas lógicas (coordenadas impares de la cuadrícula compacta) y se devuelve
	// el centro de su pasillo en la cuadrícula final.
	auto odd = [](int p_v) { return (p_v % 2 == 0) ? p_v - 1 : p_v; };
	const int step = corridor_width_ + 1;
	const int compact_w = ((width_ - 1) / step) * 2 + 1;
	const int compact_h = ((height_ - 1) / step) * 2 + 1;
	const int left = compact_to_final(1);
	const int top = compact_to_final(1);
	const int right = compact_to_final(compact_w - 2);
	const int bottom = compact_to_final(compact_h - 2);
	const int mid_x = compact_to_final(odd(compact_w / 2));
	const int mid_y = compact_to_final(odd(compact_h / 2));
	const std::array<Cell, 8> spots = {
		Cell{ left, top },
		Cell{ right, bottom },
		Cell{ right, top },
		Cell{ left, bottom },
		Cell{ mid_x, top },
		Cell{ mid_x, bottom },
		Cell{ left, mid_y },
		Cell{ right, mid_y },
	};
	std::vector<Cell> result;
	for (int i = 0; i < p_count && i < static_cast<int>(spots.size()); i++) {
		result.push_back(spots[i]);
	}
	return result;
}

bool MazeGrid::is_fully_connected() const {
	const std::vector<Cell> floors = floor_cells();
	if (floors.empty()) {
		return true;
	}
	std::vector<uint8_t> seen(walls_.size(), 0);
	std::queue<Cell> open;
	open.push(floors.front());
	seen[index(floors.front())] = 1;
	size_t reached = 1;
	while (!open.empty()) {
		const Cell c = open.front();
		open.pop();
		for (const Cell &d : DIRECTIONS) {
			const Cell n{ c.x + d.x, c.y + d.y };
			if (!is_wall(n) && !seen[index(n)]) {
				seen[index(n)] = 1;
				reached++;
				open.push(n);
			}
		}
	}
	return reached == floors.size();
}

} // namespace lv::core
