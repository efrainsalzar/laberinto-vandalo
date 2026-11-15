#include <doctest/doctest.h>

#include "core/pickup_placer.h"
#include "core/random.h"

using lv::core::Cell;
using lv::core::chebyshev_distance;
using lv::core::MazeGrid;
using lv::core::PickupPlacementOptions;
using lv::core::place_pickups;
using lv::core::Rng;

namespace {
MazeGrid game_maze(uint32_t p_seed = 5) {
	MazeGrid grid;
	MazeGrid::Options options;
	options.cells_w = 20;
	options.cells_h = 14;
	options.corridor_width = 3;
	options.vault_radius = 4;
	options.seed = p_seed;
	grid.generate(options);
	return grid;
}
} // namespace

TEST_SUITE("Rng (aleatorio portable)") {
	TEST_CASE("Misma semilla, misma secuencia") {
		Rng a(123);
		Rng b(123);
		for (int i = 0; i < 100; i++) {
			CHECK(a.next_u32() == b.next_u32());
		}
	}

	TEST_CASE("range y unit están dentro de sus límites") {
		Rng rng(7);
		for (int i = 0; i < 2000; i++) {
			const int r = rng.range(5);
			CHECK((r >= 0 && r < 5));
			const float u = rng.unit();
			CHECK((u >= 0.0f && u < 1.0f));
		}
	}

	TEST_CASE("weighted respeta los pesos e ignora los que son 0") {
		Rng rng(99);
		int counts[3] = { 0, 0, 0 };
		for (int i = 0; i < 10000; i++) {
			counts[rng.weighted({ 9.0f, 1.0f, 0.0f })]++;
		}
		CHECK(counts[2] == 0);
		CHECK(counts[0] > counts[1] * 5); // ~9 a 1
		CHECK(rng.weighted({ 0.0f, 0.0f }) == -1);
	}
}

TEST_SUITE("place_pickups") {
	const std::vector<float> WEIGHTS = { 10.0f, 10.0f, 2.0f };

	TEST_CASE("Coloca la cantidad pedida, en suelo y sin repetir casillas") {
		const MazeGrid grid = game_maze();
		PickupPlacementOptions options;
		options.count = 30;
		const auto placed = place_pickups(grid, WEIGHTS, grid.base_cells(4), options);
		REQUIRE(placed.size() == 30);
		for (size_t i = 0; i < placed.size(); i++) {
			CHECK_FALSE(grid.is_wall(placed[i].cell));
			CHECK((placed[i].kind >= 0 && placed[i].kind < 3));
			for (size_t j = i + 1; j < placed.size(); j++) {
				CHECK(placed[i].cell != placed[j].cell);
			}
		}
	}

	TEST_CASE("Respeta bóveda, bases y separación mínima") {
		const MazeGrid grid = game_maze();
		const auto bases = grid.base_cells(4);
		PickupPlacementOptions options;
		options.count = 40;
		options.min_spacing = 4;
		options.base_clearance = 6;
		const auto placed = place_pickups(grid, WEIGHTS, bases, options);
		for (size_t i = 0; i < placed.size(); i++) {
			CHECK_FALSE(grid.is_in_vault(placed[i].cell));
			for (const Cell &base : bases) {
				CHECK(chebyshev_distance(placed[i].cell, base) > 6);
			}
			for (size_t j = i + 1; j < placed.size(); j++) {
				CHECK(chebyshev_distance(placed[i].cell, placed[j].cell) >= 4);
			}
		}
	}

	TEST_CASE("Misma semilla, mismas posiciones y tipos") {
		const MazeGrid grid = game_maze();
		PickupPlacementOptions options;
		options.seed = 77;
		const auto a = place_pickups(grid, WEIGHTS, grid.base_cells(4), options);
		const auto b = place_pickups(grid, WEIGHTS, grid.base_cells(4), options);
		REQUIRE(a.size() == b.size());
		for (size_t i = 0; i < a.size(); i++) {
			CHECK(a[i].cell == b[i].cell);
			CHECK(a[i].kind == b[i].kind);
		}
	}

	TEST_CASE("Los tipos raros salen menos que los comunes") {
		int common = 0;
		int rare = 0;
		for (uint32_t seed = 1; seed <= 20; seed++) {
			const MazeGrid grid = game_maze(seed);
			PickupPlacementOptions options;
			options.seed = seed;
			for (const auto &p : place_pickups(grid, WEIGHTS, grid.base_cells(4), options)) {
				(p.kind == 2 ? rare : common)++;
			}
		}
		CHECK(rare > 0);
		CHECK(common > rare * 4); // pesos 20 a 2
	}

	TEST_CASE("Si no hay espacio, coloca menos sin fallar") {
		const MazeGrid grid = game_maze();
		PickupPlacementOptions options;
		options.count = 10000;
		options.min_spacing = 6;
		const auto placed = place_pickups(grid, WEIGHTS, grid.base_cells(4), options);
		CHECK(placed.size() > 0);
		CHECK(placed.size() < 10000);
		options.count = 0;
		CHECK(place_pickups(grid, WEIGHTS, {}, options).empty());
	}
}
