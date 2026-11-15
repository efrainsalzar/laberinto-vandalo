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
	vault_radius_ = std::max(0, p_options.vault_radius);
	corridor_width_ = std::max(1, p_options.corridor_width);

	// Paso 1: topología con el algoritmo elegido (Strategy), bucles y eliminación de callejones.
	Rng rng(p_options.seed);
	PassageGrid passages(cells_w, cells_h);
	carve_maze(p_options.algorithm, passages, rng, p_options.growing_tree_bias);
	add_loops(passages, p_options.loop_chance, rng);
	braid(passages, p_options.braid_chance, rng);

	metrics_ = Metrics();
	metrics_.algorithm = p_options.algorithm;
	metrics_.cells = passages.cell_count();
	for (int i = 0; i < passages.cell_count(); i++) {
		const int degree = passages.degree(i);
		metrics_.dead_ends += degree == 1 ? 1 : 0;
		metrics_.junctions += degree >= 3 ? 1 : 0;
	}
	metrics_.dead_end_ratio = static_cast<float>(metrics_.dead_ends) / static_cast<float>(metrics_.cells);

	// Pasos 2-4: dibujar en casillas, ensanchar y abrir la bóveda.
	rasterize(passages);
	widen_corridors(corridor_width_);
	open_vault();
	passages_ = passages;
	sync_passages_with_tiles();
	compute_protected_passages();

	const std::vector<Cell> bases = base_cells(1);
	metrics_.route_length = bases.empty() ? 0 : path_length(bases[0], center());
}

// Paso 2: cuadrícula compacta: celda (x, y) -> casilla (2x+1, 2y+1); un pasaje abre la casilla entre ambas.
void MazeGrid::rasterize(const PassageGrid &p_passages) {
	width_ = p_passages.width() * 2 + 1;
	height_ = p_passages.height() * 2 + 1;
	walls_.assign(static_cast<size_t>(width_) * height_, 1);
	for (int i = 0; i < p_passages.cell_count(); i++) {
		const Cell c = p_passages.cell(i);
		set_wall({ c.x * 2 + 1, c.y * 2 + 1 }, false);
		if (c.x + 1 < p_passages.width() && p_passages.is_open(i, i + 1)) {
			set_wall({ c.x * 2 + 2, c.y * 2 + 1 }, false);
		}
		if (c.y + 1 < p_passages.height() && p_passages.is_open(i, i + p_passages.width())) {
			set_wall({ c.x * 2 + 1, c.y * 2 + 2 }, false);
		}
	}
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

std::vector<TileRect> MazeGrid::base_zones(int p_count) const {
	// base_cells devuelve la casilla k*step + 1 + cw/2; el pasillo empieza en k*step + 1.
	std::vector<TileRect> result;
	const int half = corridor_width_ / 2;
	for (const Cell &c : base_cells(p_count)) {
		result.push_back({ c.x - half, c.y - half, corridor_width_, corridor_width_ });
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

int MazeGrid::path_length(const Cell &p_from, const Cell &p_to) const {
	if (is_wall(p_from) || is_wall(p_to)) {
		return -1;
	}
	std::vector<int> distance(walls_.size(), -1);
	std::queue<Cell> open;
	open.push(p_from);
	distance[index(p_from)] = 0;
	while (!open.empty()) {
		const Cell c = open.front();
		open.pop();
		if (c == p_to) {
			return distance[index(c)];
		}
		for (const Cell &d : DIRECTIONS) {
			const Cell n{ c.x + d.x, c.y + d.y };
			if (!is_wall(n) && distance[index(n)] < 0) {
				distance[index(n)] = distance[index(c)] + 1;
				open.push(n);
			}
		}
	}
	return -1;
}

// --- Mapa cambiante -------------------------------------------------------------------------

std::vector<Cell> MazeGrid::passage_tiles(int p_a, int p_b) const {
	std::vector<Cell> tiles;
	if (!passages_.are_adjacent(p_a, p_b)) {
		return tiles;
	}
	if (p_a > p_b) {
		std::swap(p_a, p_b);
	}
	const Cell a = passages_.cell(p_a);
	const int step = corridor_width_ + 1;
	if (p_b == p_a + 1) { // vecino al este: columna de pared vertical
		const int x = (a.x + 1) * step;
		for (int i = 0; i < corridor_width_; i++) {
			tiles.push_back({ x, a.y * step + 1 + i });
		}
	} else { // vecino al sur: fila de pared horizontal
		const int y = (a.y + 1) * step;
		for (int i = 0; i < corridor_width_; i++) {
			tiles.push_back({ a.x * step + 1 + i, y });
		}
	}
	return tiles;
}

Cell MazeGrid::cell_center(int p_cell) const {
	const Cell c = passages_.cell(p_cell);
	const int step = corridor_width_ + 1;
	return { c.x * step + 1 + corridor_width_ / 2, c.y * step + 1 + corridor_width_ / 2 };
}

int MazeGrid::cell_index_of(const Cell &p_tile) const {
	const int step = corridor_width_ + 1;
	const int cx = std::clamp((p_tile.x - 1) / step, 0, passages_.width() - 1);
	const int cy = std::clamp((p_tile.y - 1) / step, 0, passages_.height() - 1);
	return passages_.index({ cx, cy });
}

bool MazeGrid::compute_is_protected(int p_a, int p_b) const {
	const std::vector<Cell> bases = base_cells(8);
	const int base_guard = corridor_width_ + 1; // la celda de la base y sus pasajes
	for (const Cell &tile : passage_tiles(p_a, p_b)) {
		if (tile.x <= 0 || tile.y <= 0 || tile.x >= width_ - 1 || tile.y >= height_ - 1) {
			return true;
		}
		if (vault_radius_ > 0 && chebyshev_distance(tile, center()) <= vault_radius_ + 1) {
			return true;
		}
		for (const Cell &base : bases) {
			if (chebyshev_distance(tile, base) <= base_guard) {
				return true;
			}
		}
	}
	return false;
}

void MazeGrid::compute_protected_passages() {
	protected_east_.assign(static_cast<size_t>(passages_.cell_count()), 0);
	protected_south_.assign(static_cast<size_t>(passages_.cell_count()), 0);
	for (int i = 0; i < passages_.cell_count(); i++) {
		const Cell c = passages_.cell(i);
		if (c.x + 1 < passages_.width()) {
			protected_east_[i] = compute_is_protected(i, i + 1) ? 1 : 0;
		}
		if (c.y + 1 < passages_.height()) {
			protected_south_[i] = compute_is_protected(i, i + passages_.width()) ? 1 : 0;
		}
	}
}

bool MazeGrid::is_protected_passage(int p_a, int p_b) const {
	if (!passages_.are_adjacent(p_a, p_b)) {
		return true;
	}
	if (p_a > p_b) {
		std::swap(p_a, p_b);
	}
	return (p_b == p_a + 1 ? protected_east_[p_a] : protected_south_[p_a]) != 0;
}

bool MazeGrid::can_open_passage(int p_a, int p_b) const {
	return passages_.are_adjacent(p_a, p_b) && !passages_.is_open(p_a, p_b) && !is_protected_passage(p_a, p_b);
}

bool MazeGrid::can_close_passage(int p_a, int p_b) const {
	if (!passages_.are_adjacent(p_a, p_b) || !passages_.is_open(p_a, p_b) || is_protected_passage(p_a, p_b)) {
		return false;
	}
	return passages_.has_alternative_path(p_a, p_b);
}

std::vector<Cell> MazeGrid::set_passage(int p_a, int p_b, bool p_open) {
	std::vector<Cell> changed;
	if (!passages_.are_adjacent(p_a, p_b)) {
		return changed;
	}
	if (p_open) {
		passages_.carve(p_a, p_b);
	} else {
		passages_.close(p_a, p_b);
	}
	for (const Cell &tile : passage_tiles(p_a, p_b)) {
		if (is_wall(tile) == p_open) {
			set_wall(tile, !p_open);
			changed.push_back(tile);
		}
	}
	return changed;
}

Cell MazeGrid::nearest_floor(const Cell &p_cell) const {
	if (!is_wall(p_cell)) {
		return p_cell;
	}
	std::vector<uint8_t> seen(walls_.size(), 0);
	std::queue<Cell> open;
	Cell start{ std::clamp(p_cell.x, 0, width_ - 1), std::clamp(p_cell.y, 0, height_ - 1) };
	open.push(start);
	seen[index(start)] = 1;
	while (!open.empty()) {
		const Cell c = open.front();
		open.pop();
		if (!is_wall(c)) {
			return c;
		}
		for (const Cell &d : DIRECTIONS) {
			const Cell n{ c.x + d.x, c.y + d.y };
			if (in_bounds(n) && !seen[index(n)]) {
				seen[index(n)] = 1;
				open.push(n);
			}
		}
	}
	return p_cell;
}

void MazeGrid::sync_passages_with_tiles() {
	for (int i = 0; i < passages_.cell_count(); i++) {
		int n[4];
		const int count = passages_.neighbors(i, n);
		for (int k = 0; k < count; k++) {
			if (n[k] < i || passages_.is_open(i, n[k])) {
				continue;
			}
			bool all_floor = true;
			for (const Cell &tile : passage_tiles(i, n[k])) {
				all_floor = all_floor && !is_wall(tile);
			}
			if (all_floor) {
				passages_.carve(i, n[k]);
			}
		}
	}
}

const char *maze_difficulty_name(MazeDifficulty p_difficulty) {
	switch (p_difficulty) {
		case MazeDifficulty::Easy:
			return "Fácil";
		case MazeDifficulty::Normal:
			return "Normal";
		case MazeDifficulty::Hard:
			return "Difícil";
		case MazeDifficulty::Extreme:
			return "Extremo";
		default:
			return "?";
	}
}

MazeGrid::Options maze_preset(MazeDifficulty p_difficulty) {
	MazeGrid::Options o;
	o.corridor_width = 3;
	o.vault_radius = 4;
	switch (p_difficulty) {
		case MazeDifficulty::Easy:
			// Pasillos largos, pocos callejones y muchas rutas: difícil perderse.
			o.cells_w = 16;
			o.cells_h = 11;
			o.algorithm = MazeAlgorithm::HuntAndKill;
			o.loop_chance = 0.15f;
			o.braid_chance = 0.6f;
			break;
		case MazeDifficulty::Hard:
			// Uniforme y grande: muchos cruces y callejones, pocas rutas alternativas.
			o.cells_w = 26;
			o.cells_h = 18;
			o.algorithm = MazeAlgorithm::Wilson;
			o.loop_chance = 0.03f;
			o.braid_chance = 0.05f;
			break;
		case MazeDifficulty::Extreme:
			// Muy ramificado, enorme, pasillos estrechos y sin bucles: confunde al máximo.
			o.cells_w = 32;
			o.cells_h = 22;
			o.corridor_width = 2;
			o.vault_radius = 3;
			o.algorithm = MazeAlgorithm::Prim;
			o.loop_chance = 0.0f;
			o.braid_chance = 0.0f;
			break;
		case MazeDifficulty::Normal:
		default:
			// Mezcla equilibrada entre pasillos largos y ramificaciones.
			o.cells_w = 20;
			o.cells_h = 14;
			o.algorithm = MazeAlgorithm::GrowingTree;
			o.growing_tree_bias = 0.5f;
			o.loop_chance = 0.08f;
			o.braid_chance = 0.25f;
			break;
	}
	return o;
}

} // namespace lv::core
