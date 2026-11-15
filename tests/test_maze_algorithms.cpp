#include <doctest/doctest.h>

#include "core/maze_algorithms.h"
#include "core/maze_grid.h"

using lv::core::braid;
using lv::core::carve_maze;
using lv::core::maze_preset;
using lv::core::MazeAlgorithm;
using lv::core::MazeDifficulty;
using lv::core::MazeGrid;
using lv::core::PassageGrid;
using lv::core::Rng;

namespace {
constexpr int ALGORITHM_COUNT = static_cast<int>(MazeAlgorithm::Count);

float average_dead_end_ratio(MazeAlgorithm p_algorithm) {
	float total = 0.0f;
	for (uint32_t seed = 1; seed <= 20; seed++) {
		PassageGrid grid(20, 14);
		Rng rng(seed);
		carve_maze(p_algorithm, grid, rng);
		int dead = 0;
		for (int i = 0; i < grid.cell_count(); i++) {
			dead += grid.degree(i) == 1 ? 1 : 0;
		}
		total += static_cast<float>(dead) / grid.cell_count();
	}
	return total / 20.0f;
}

MazeGrid::Metrics average_preset_metrics(MazeDifficulty p_difficulty) {
	MazeGrid::Metrics sum;
	for (uint32_t seed = 1; seed <= 20; seed++) {
		MazeGrid::Options options = maze_preset(p_difficulty);
		options.seed = seed;
		MazeGrid grid;
		grid.generate(options);
		sum.dead_end_ratio += grid.metrics().dead_end_ratio / 20.0f;
		sum.route_length += grid.metrics().route_length / 20;
		sum.cells = grid.metrics().cells;
	}
	return sum;
}
} // namespace

TEST_SUITE("Algoritmos de laberinto") {
	TEST_CASE("Cada algoritmo genera un laberinto perfecto (árbol conectado)") {
		for (int a = 0; a < ALGORITHM_COUNT; a++) {
			for (uint32_t seed = 1; seed <= 10; seed++) {
				CAPTURE(lv::core::maze_algorithm_name(static_cast<MazeAlgorithm>(a)));
				CAPTURE(seed);
				PassageGrid grid(17, 11);
				Rng rng(seed);
				carve_maze(static_cast<MazeAlgorithm>(a), grid, rng);
				CHECK(grid.is_connected());
				CHECK(grid.passage_count() == grid.cell_count() - 1); // árbol: sin bucles
			}
		}
	}

	TEST_CASE("Cada algoritmo es determinista con la misma semilla") {
		for (int a = 0; a < ALGORITHM_COUNT; a++) {
			CAPTURE(a);
			PassageGrid g1(12, 9);
			PassageGrid g2(12, 9);
			Rng r1(55);
			Rng r2(55);
			carve_maze(static_cast<MazeAlgorithm>(a), g1, r1);
			carve_maze(static_cast<MazeAlgorithm>(a), g2, r2);
			bool same = true;
			for (int i = 0; i < g1.cell_count(); i++) {
				int n[4];
				const int count = g1.neighbors(i, n);
				for (int k = 0; k < count; k++) {
					same = same && g1.is_open(i, n[k]) == g2.is_open(i, n[k]);
				}
			}
			CHECK(same);
		}
	}

	TEST_CASE("Las personalidades se notan: Prim tiene muchos más callejones que Backtracking") {
		const float backtracker = average_dead_end_ratio(MazeAlgorithm::RecursiveBacktracker);
		const float hunt = average_dead_end_ratio(MazeAlgorithm::HuntAndKill);
		const float wilson = average_dead_end_ratio(MazeAlgorithm::Wilson);
		const float prim = average_dead_end_ratio(MazeAlgorithm::Prim);
		CHECK(backtracker < 0.2f);
		CHECK(hunt < 0.2f);
		CHECK(wilson > 0.25f);
		CHECK(prim > wilson);
	}

	TEST_CASE("Braid al 100 % elimina todos los callejones y sigue conectado") {
		PassageGrid grid(15, 10);
		Rng rng(4);
		carve_maze(MazeAlgorithm::Prim, grid, rng);
		braid(grid, 1.0f, rng);
		CHECK(grid.is_connected());
		for (int i = 0; i < grid.cell_count(); i++) {
			CHECK(grid.degree(i) >= 2);
		}
	}

	TEST_CASE("Todos los niveles generan mapas conectados con ruta de la base al centro") {
		for (int d = 0; d < static_cast<int>(MazeDifficulty::Count); d++) {
			for (uint32_t seed = 1; seed <= 5; seed++) {
				CAPTURE(lv::core::maze_difficulty_name(static_cast<MazeDifficulty>(d)));
				MazeGrid::Options options = maze_preset(static_cast<MazeDifficulty>(d));
				options.seed = seed;
				MazeGrid grid;
				grid.generate(options);
				CHECK(grid.is_fully_connected());
				CHECK(grid.metrics().route_length > 0);
			}
		}
	}

	TEST_CASE("Los niveles están ordenados por dificultad (tamaño, callejones y ruta)") {
		const auto easy = average_preset_metrics(MazeDifficulty::Easy);
		const auto normal = average_preset_metrics(MazeDifficulty::Normal);
		const auto hard = average_preset_metrics(MazeDifficulty::Hard);
		const auto extreme = average_preset_metrics(MazeDifficulty::Extreme);
		MESSAGE("callejones %: ", easy.dead_end_ratio, " < ", normal.dead_end_ratio, " < ", hard.dead_end_ratio, " < ", extreme.dead_end_ratio);
		MESSAGE("ruta (casillas): ", easy.route_length, " < ", normal.route_length, " < ", hard.route_length, " < ", extreme.route_length);
		CHECK(easy.cells < normal.cells);
		CHECK(normal.cells < hard.cells);
		CHECK(hard.cells < extreme.cells);
		CHECK(easy.dead_end_ratio < normal.dead_end_ratio);
		CHECK(normal.dead_end_ratio < hard.dead_end_ratio);
		CHECK(hard.dead_end_ratio < extreme.dead_end_ratio);
		CHECK(easy.route_length < normal.route_length);
		CHECK(normal.route_length < hard.route_length);
	}
}
