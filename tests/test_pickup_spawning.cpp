#include <doctest/doctest.h>

#include "core/pickup_spawning.h"

using lv::core::Cell;
using lv::core::chebyshev_distance;
using lv::core::GridPoint;
using lv::core::KeepDensityRule;
using lv::core::MazeGrid;
using lv::core::pick_spawn_cell;
using lv::core::Rng;
using lv::core::SpawnCellOptions;
using lv::core::SpawnObserver;

namespace {
MazeGrid game_maze() {
	MazeGrid grid;
	MazeGrid::Options options;
	options.cells_w = 20;
	options.cells_h = 14;
	options.corridor_width = 3;
	options.vault_radius = 4;
	options.seed = 21;
	grid.generate(options);
	return grid;
}
} // namespace

TEST_SUITE("pick_spawn_cell") {
	TEST_CASE("Nunca aparece donde un observador lo ve, ni en pared, bóveda o cerca de bases/objetos") {
		const MazeGrid grid = game_maze();
		const auto bases = grid.base_cells(4);
		const std::vector<Cell> occupied = { { 30, 20 }, { 50, 30 } };
		const std::vector<SpawnObserver> observers = { { { 2.5f, 2.5f }, 5.0f }, { { 40.5f, 28.5f }, 7.0f } };
		SpawnCellOptions options;
		Rng rng(3);
		int found = 0;
		for (int i = 0; i < 300; i++) {
			const auto cell = pick_spawn_cell(grid, occupied, bases, observers, options, rng);
			if (!cell) {
				continue;
			}
			found++;
			CHECK_FALSE(grid.is_wall(*cell));
			CHECK_FALSE(grid.is_in_vault(*cell));
			for (const Cell &base : bases) {
				CHECK(chebyshev_distance(*cell, base) > options.base_clearance);
			}
			for (const Cell &other : occupied) {
				CHECK(chebyshev_distance(*cell, other) >= options.min_spacing);
			}
			const GridPoint center{ cell->x + 0.5f, cell->y + 0.5f };
			for (const SpawnObserver &o : observers) {
				CHECK_FALSE(lv::core::can_see(grid, o.position, center, o.vision_radius + options.observer_margin));
			}
		}
		CHECK(found > 250); // casi siempre encuentra lugar en un mapa grande
	}

	TEST_CASE("Determinista con la misma semilla") {
		const MazeGrid grid = game_maze();
		Rng a(8);
		Rng b(8);
		for (int i = 0; i < 20; i++) {
			const auto ca = pick_spawn_cell(grid, {}, {}, {}, {}, a);
			const auto cb = pick_spawn_cell(grid, {}, {}, {}, {}, b);
			REQUIRE(ca.has_value() == cb.has_value());
			if (ca) {
				CHECK(*ca == *cb);
			}
		}
	}

	TEST_CASE("Si no hay lugar posible devuelve nullopt") {
		const MazeGrid grid = game_maze();
		SpawnCellOptions options;
		options.base_clearance = 1000; // todo el mapa queda "cerca de una base"
		Rng rng(1);
		CHECK_FALSE(pick_spawn_cell(grid, {}, { { 1, 1 } }, {}, options, rng).has_value());
	}
}

TEST_SUITE("KeepDensityRule") {
	TEST_CASE("Si está lleno no aparece nada") {
		KeepDensityRule rule;
		rule.target_count = 10;
		CHECK(rule.update(10, 100.0) == 0);
		CHECK(rule.update(12, 100.0) == 0);
	}

	TEST_CASE("Si falta uno, aparece tras el retraso") {
		KeepDensityRule rule;
		rule.target_count = 10;
		rule.respawn_delay = 3.0f;
		CHECK(rule.update(9, 1.0) == 0);
		CHECK(rule.update(9, 1.0) == 0);
		CHECK(rule.update(9, 1.5) == 1);
	}

	TEST_CASE("Repone de a uno por retraso y nunca pasa del objetivo") {
		KeepDensityRule rule;
		rule.target_count = 10;
		rule.respawn_delay = 2.0f;
		CHECK(rule.update(5, 7.0) == 3); // 7 s = 3 apariciones (sobran 1 s)
		CHECK(rule.update(8, 10.0) == 2); // solo faltan 2
	}

	TEST_CASE("Al llenarse, el reloj se reinicia") {
		KeepDensityRule rule;
		rule.target_count = 3;
		rule.respawn_delay = 2.0f;
		rule.update(2, 1.9);
		rule.update(3, 0.1); // se llenó por otro motivo
		CHECK(rule.update(2, 1.0) == 0); // empieza a contar de nuevo
	}
}
