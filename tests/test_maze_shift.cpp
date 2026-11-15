#include <doctest/doctest.h>

#include "core/maze_shift.h"

using lv::core::Cell;
using lv::core::maze_preset;
using lv::core::MazeDifficulty;
using lv::core::MazeGrid;
using lv::core::PassageChange;
using lv::core::plan_maze_shift;
using lv::core::Rng;

namespace {
MazeGrid make(MazeDifficulty p_difficulty, uint32_t p_seed) {
	MazeGrid grid;
	MazeGrid::Options options = maze_preset(p_difficulty);
	options.seed = p_seed;
	grid.generate(options);
	return grid;
}

void apply(MazeGrid &r_grid, const std::vector<PassageChange> &p_plan) {
	for (const PassageChange &c : p_plan) {
		r_grid.set_passage(c.a, c.b, c.open);
	}
}
} // namespace

TEST_SUITE("Laberinto vivo") {
	TEST_CASE("Tras generar, la topología coincide con las casillas (incluida la bóveda)") {
		const MazeGrid grid = make(MazeDifficulty::Normal, 3);
		const auto &p = grid.passages();
		for (int i = 0; i < p.cell_count(); i++) {
			int n[4];
			const int count = p.neighbors(i, n);
			for (int k = 0; k < count; k++) {
				bool all_floor = true;
				for (const Cell &t : grid.passage_tiles(i, n[k])) {
					all_floor = all_floor && !grid.is_wall(t);
				}
				CHECK(all_floor == p.is_open(i, n[k]));
			}
		}
	}

	TEST_CASE("Abrir y cerrar un pasaje cambia exactamente sus casillas") {
		MazeGrid grid = make(MazeDifficulty::Normal, 8);
		const auto &p = grid.passages();
		for (int i = 0; i < p.cell_count(); i++) {
			int n[4];
			const int count = p.neighbors(i, n);
			for (int k = 0; k < count; k++) {
				if (grid.can_open_passage(i, n[k])) {
					const auto tiles = grid.passage_tiles(i, n[k]);
					CHECK(grid.set_passage(i, n[k], true).size() == tiles.size());
					for (const Cell &t : tiles) {
						CHECK_FALSE(grid.is_wall(t));
					}
					CHECK(grid.set_passage(i, n[k], false).size() == tiles.size());
					for (const Cell &t : tiles) {
						CHECK(grid.is_wall(t));
					}
					return;
				}
			}
		}
		FAIL("no se encontró ningún pasaje para abrir");
	}

	TEST_CASE("Muchos movimientos seguidos nunca desconectan el mapa ni cambian la cantidad de pasajes") {
		for (int d = 0; d < static_cast<int>(MazeDifficulty::Count); d++) {
			CAPTURE(d);
			MazeGrid grid = make(static_cast<MazeDifficulty>(d), 11);
			Rng rng(99);
			const int passages_before = grid.passages().passage_count();
			for (int shift = 0; shift < 60; shift++) {
				const auto plan = plan_maze_shift(grid, 3, rng);
				int opens = 0;
				int closes = 0;
				for (const auto &c : plan) {
					(c.open ? opens : closes)++;
				}
				CHECK(opens == closes);
				apply(grid, plan);
				REQUIRE(grid.passages().is_connected());
			}
			CHECK(grid.is_fully_connected());
			CHECK(grid.passages().passage_count() == passages_before);
			CHECK(grid.path_length(grid.base_cells(1)[0], grid.center()) > 0);
		}
	}

	TEST_CASE("Nunca toca la bóveda, las bases ni el borde") {
		MazeGrid grid = make(MazeDifficulty::Hard, 5);
		Rng rng(1);
		for (int shift = 0; shift < 40; shift++) {
			const auto plan = plan_maze_shift(grid, 4, rng);
			for (const auto &c : plan) {
				CHECK_FALSE(grid.is_protected_passage(c.a, c.b));
			}
			apply(grid, plan);
		}
		const Cell center = grid.center();
		for (int y = -4; y <= 4; y++) {
			for (int x = -4; x <= 4; x++) {
				CHECK_FALSE(grid.is_wall({ center.x + x, center.y + y }));
			}
		}
		for (const Cell &b : grid.base_cells(4)) {
			CHECK_FALSE(grid.is_wall(b));
		}
	}

	TEST_CASE("Cancelar cierres (alguien estaba ahí) mantiene el mapa conectado") {
		MazeGrid grid = make(MazeDifficulty::Extreme, 2);
		Rng rng(7);
		for (int shift = 0; shift < 30; shift++) {
			auto plan = plan_maze_shift(grid, 3, rng);
			for (const auto &c : plan) {
				if (c.open || (shift + c.a) % 2 == 0) { // se aplica la mitad de los cierres
					grid.set_passage(c.a, c.b, c.open);
				}
			}
			REQUIRE(grid.passages().is_connected());
		}
	}

	TEST_CASE("Planificación determinista con la misma semilla") {
		const MazeGrid grid = make(MazeDifficulty::Normal, 4);
		Rng a(5);
		Rng b(5);
		const auto pa = plan_maze_shift(grid, 3, a);
		const auto pb = plan_maze_shift(grid, 3, b);
		REQUIRE(pa.size() == pb.size());
		for (size_t i = 0; i < pa.size(); i++) {
			CHECK(pa[i].a == pb[i].a);
			CHECK(pa[i].b == pb[i].b);
			CHECK(pa[i].open == pb[i].open);
		}
	}

	TEST_CASE("nearest_floor encuentra el suelo más cercano") {
		const MazeGrid grid = make(MazeDifficulty::Normal, 1);
		const Cell f = grid.nearest_floor({ 0, 0 });
		CHECK_FALSE(grid.is_wall(f));
		CHECK(lv::core::chebyshev_distance(f, { 0, 0 }) <= 2);
		CHECK(grid.nearest_floor(grid.center()) == grid.center());
	}
}
