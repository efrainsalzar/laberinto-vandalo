#include <doctest/doctest.h>

#include "core/line_of_sight.h"

using lv::core::can_see;
using lv::core::GridPoint;
using lv::core::has_line_of_sight;
using lv::core::MazeGrid;

namespace {
// Laberinto real con una bóveda grande: el centro es una sala abierta sin paredes internas,
// y alrededor hay pasillos y paredes para los casos de bloqueo.
MazeGrid open_room() {
	MazeGrid grid;
	MazeGrid::Options options;
	options.cells_w = 6;
	options.cells_h = 6;
	options.corridor_width = 3;
	options.vault_radius = 8;
	options.seed = 1;
	grid.generate(options);
	return grid;
}

GridPoint center_of(int p_x, int p_y) {
	return { p_x + 0.5f, p_y + 0.5f };
}
} // namespace

TEST_SUITE("Línea de visión") {
	TEST_CASE("En una sala abierta se ve en todas las direcciones") {
		const MazeGrid grid = open_room();
		const auto c = grid.center();
		const GridPoint from = center_of(c.x, c.y);
		for (int d = -6; d <= 6; d += 3) {
			CHECK(has_line_of_sight(grid, from, center_of(c.x + d, c.y)));
			CHECK(has_line_of_sight(grid, from, center_of(c.x, c.y + d)));
			CHECK(has_line_of_sight(grid, from, center_of(c.x + d, c.y + d)));
		}
	}

	TEST_CASE("Una pared en medio bloquea la vista") {
		const MazeGrid grid = open_room();
		// El borde exterior siempre es pared: desde dentro no se ve "a través" de él.
		CHECK_FALSE(has_line_of_sight(grid, center_of(2, 2), center_of(2, -3)));
		// Buscar una pared con suelo a ambos lados en horizontal y comprobar que bloquea.
		bool checked = false;
		for (int y = 1; y < grid.height() - 1 && !checked; y++) {
			for (int x = 1; x < grid.width() - 1 && !checked; x++) {
				if (grid.is_wall({ x, y }) && !grid.is_wall({ x - 1, y }) && !grid.is_wall({ x + 1, y })) {
					CHECK_FALSE(has_line_of_sight(grid, center_of(x - 1, y), center_of(x + 1, y)));
					checked = true;
				}
			}
		}
		CHECK(checked);
	}

	TEST_CASE("Desde o hacia una pared no hay visión") {
		const MazeGrid grid = open_room();
		CHECK_FALSE(has_line_of_sight(grid, center_of(0, 0), center_of(2, 2)));
		CHECK_FALSE(has_line_of_sight(grid, center_of(2, 2), center_of(0, 0)));
	}

	TEST_CASE("La misma casilla siempre se ve") {
		const MazeGrid grid = open_room();
		CHECK(has_line_of_sight(grid, { 2.2f, 2.2f }, { 2.8f, 2.9f }));
	}

	TEST_CASE("can_see respeta el radio") {
		const MazeGrid grid = open_room();
		const auto c = grid.center();
		const GridPoint from = center_of(c.x, c.y);
		CHECK(can_see(grid, from, center_of(c.x + 4, c.y), 5.0f));
		CHECK_FALSE(can_see(grid, from, center_of(c.x + 6, c.y), 5.0f));
	}

	TEST_CASE("Es simétrica: si A ve a B, B ve a A") {
		const MazeGrid grid = open_room();
		const auto floors = grid.floor_cells();
		for (size_t i = 0; i < floors.size(); i += 37) {
			for (size_t j = 0; j < floors.size(); j += 53) {
				const GridPoint a = center_of(floors[i].x, floors[i].y);
				const GridPoint b = center_of(floors[j].x, floors[j].y);
				CHECK(has_line_of_sight(grid, a, b) == has_line_of_sight(grid, b, a));
			}
		}
	}
}
