#include <doctest/doctest.h>

#include "core/maze_grid.h"

using lv::core::Cell;
using lv::core::MazeGrid;

namespace {
MazeGrid make_maze(uint32_t p_seed, float p_loops = 0.12f) {
	MazeGrid grid;
	MazeGrid::Options options;
	options.cells_w = 12;
	options.cells_h = 9;
	options.seed = p_seed;
	options.loop_chance = p_loops;
	grid.generate(options);
	return grid;
}
} // namespace

TEST_SUITE("MazeGrid") {
	TEST_CASE("Las dimensiones son 2*celdas+1") {
		const MazeGrid grid = make_maze(1);
		CHECK(grid.width() == 25);
		CHECK(grid.height() == 19);
	}

	TEST_CASE("El borde exterior siempre es pared") {
		const MazeGrid grid = make_maze(7);
		for (int x = 0; x < grid.width(); x++) {
			CHECK(grid.is_wall({ x, 0 }));
			CHECK(grid.is_wall({ x, grid.height() - 1 }));
		}
		for (int y = 0; y < grid.height(); y++) {
			CHECK(grid.is_wall({ 0, y }));
			CHECK(grid.is_wall({ grid.width() - 1, y }));
		}
	}

	TEST_CASE("Fuera de los límites cuenta como pared") {
		const MazeGrid grid = make_maze(1);
		CHECK(grid.is_wall({ -1, 0 }));
		CHECK(grid.is_wall({ grid.width(), 0 }));
	}

	TEST_CASE("Todo el suelo está conectado (con y sin bucles, varias semillas)") {
		for (uint32_t seed = 1; seed <= 50; seed++) {
			CAPTURE(seed);
			CHECK(make_maze(seed, 0.0f).is_fully_connected());
			CHECK(make_maze(seed, 0.3f).is_fully_connected());
		}
	}

	TEST_CASE("Misma semilla genera el mismo laberinto; otra semilla, uno distinto") {
		const MazeGrid a = make_maze(42);
		const MazeGrid b = make_maze(42);
		const MazeGrid c = make_maze(43);
		bool same = true;
		bool different = false;
		for (int y = 0; y < a.height(); y++) {
			for (int x = 0; x < a.width(); x++) {
				same = same && (a.is_wall({ x, y }) == b.is_wall({ x, y }));
				different = different || (a.is_wall({ x, y }) != c.is_wall({ x, y }));
			}
		}
		CHECK(same);
		CHECK(different);
	}

	TEST_CASE("Los bucles abren más suelo que un laberinto perfecto") {
		const size_t perfect = make_maze(5, 0.0f).floor_cells().size();
		const size_t looped = make_maze(5, 0.5f).floor_cells().size();
		CHECK(looped > perfect);
	}

	TEST_CASE("La bóveda central está abierta") {
		const MazeGrid grid = make_maze(3);
		const Cell c = grid.center();
		for (int dy = -2; dy <= 2; dy++) {
			for (int dx = -2; dx <= 2; dx++) {
				CHECK_FALSE(grid.is_wall({ c.x + dx, c.y + dy }));
				CHECK(grid.is_in_vault({ c.x + dx, c.y + dy }));
			}
		}
		CHECK_FALSE(grid.is_in_vault({ 1, 1 }));
	}

	TEST_CASE("Las bases están en suelo, son distintas y empiezan por las esquinas") {
		const MazeGrid grid = make_maze(9);
		const auto bases = grid.base_cells(8);
		REQUIRE(bases.size() == 8);
		CHECK(bases[0] == Cell{ 1, 1 });
		CHECK(bases[1] == Cell{ grid.width() - 2, grid.height() - 2 });
		for (size_t i = 0; i < bases.size(); i++) {
			CHECK_FALSE(grid.is_wall(bases[i]));
			for (size_t j = i + 1; j < bases.size(); j++) {
				CHECK(bases[i] != bases[j]);
			}
		}
		CHECK(grid.base_cells(20).size() == 8);
	}

	TEST_CASE("Pasillos anchos: dimensiones, paredes de 1 casilla y conectividad") {
		for (int corridor = 2; corridor <= 4; corridor++) {
			CAPTURE(corridor);
			MazeGrid grid;
			MazeGrid::Options options;
			options.cells_w = 10;
			options.cells_h = 7;
			options.corridor_width = corridor;
			options.vault_radius = corridor + 1;
			options.seed = 11;
			grid.generate(options);
			CHECK(grid.width() == 10 * (corridor + 1) + 1);
			CHECK(grid.height() == 7 * (corridor + 1) + 1);
			CHECK(grid.is_fully_connected());
			// La primera celda es un bloque abierto de corridor x corridor.
			for (int y = 1; y <= corridor; y++) {
				for (int x = 1; x <= corridor; x++) {
					CHECK_FALSE(grid.is_wall({ x, y }));
				}
			}
			for (const Cell &base : grid.base_cells(8)) {
				CHECK_FALSE(grid.is_wall(base));
			}
		}
	}
}