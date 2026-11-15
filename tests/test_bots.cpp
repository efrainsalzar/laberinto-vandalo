#include <doctest/doctest.h>

#include "core/bot_brain.h"
#include "core/maze_grid.h"
#include "core/nav_path.h"

using lv::core::BotBrain;
using lv::core::BotGoal;
using lv::core::BotTuning;
using lv::core::BotWorld;
using lv::core::Cell;
using lv::core::MazeGrid;
using lv::core::PassageGrid;
using lv::core::Rng;

namespace {
// Cuadrícula 4x4 totalmente abierta (todas las celdas vecinas conectadas).
PassageGrid open_grid(int p_w = 4, int p_h = 4) {
	PassageGrid g(p_w, p_h);
	for (int i = 0; i < g.cell_count(); i++) {
		int n[4];
		const int count = g.neighbors(i, n);
		for (int k = 0; k < count; k++) {
			g.carve(i, n[k]);
		}
	}
	return g;
}

// Cada paso del camino es entre vecinas con pasaje abierto.
bool is_valid_path(const PassageGrid &p_g, const std::vector<int> &p_path) {
	for (size_t i = 1; i < p_path.size(); i++) {
		if (!p_g.are_adjacent(p_path[i - 1], p_path[i]) || !p_g.is_open(p_path[i - 1], p_path[i])) {
			return false;
		}
	}
	return true;
}

BotWorld world_at(Cell p_self) {
	BotWorld w;
	w.self = p_self;
	w.base = { 1, 1, 3, 3 };
	w.trophy_available = true;
	w.trophy = { 40, 30 };
	w.explore_target = { 10, 10 };
	return w;
}

BotTuning no_noise() {
	BotTuning t;
	t.variation = 0.0f;
	t.reaction_time = 0.5f;
	t.think_interval = 0.2f;
	return t;
}
} // namespace

TEST_SUITE("Bots: navegación (A*)") {
	TEST_CASE("Camino más corto en una cuadrícula abierta (Manhattan + 1 celdas)") {
		const PassageGrid g = open_grid();
		const auto path = find_path(g, g.index({ 0, 0 }), g.index({ 3, 2 }));
		REQUIRE(path.size() == 6);
		CHECK(path.front() == g.index({ 0, 0 }));
		CHECK(path.back() == g.index({ 3, 2 }));
		CHECK(is_valid_path(g, path));
	}

	TEST_CASE("Respeta las paredes: rodea un pasaje cerrado") {
		PassageGrid g = open_grid(3, 1); // 0 - 1 - 2 en línea
		g.close(0, 1);
		CHECK(find_path(g, 0, 2).empty()); // sin otra ruta: no hay camino
		PassageGrid g2 = open_grid(3, 2);
		g2.close(0, 1); // obliga a bajar y volver a subir
		const auto path = find_path(g2, 0, 2);
		CHECK(path.size() == 5);
		CHECK(is_valid_path(g2, path));
	}

	TEST_CASE("Mismo inicio y destino; índices inválidos") {
		const PassageGrid g = open_grid();
		CHECK(find_path(g, 5, 5) == std::vector<int>{ 5 });
		CHECK(find_path(g, -1, 3).empty());
		CHECK(find_path(g, 0, 99).empty());
	}

	TEST_CASE("Los costos extra desvían el camino (listo para terrenos)") {
		const PassageGrid g = open_grid(3, 3);
		std::vector<float> cost(static_cast<size_t>(g.cell_count()), 0.0f);
		cost[g.index({ 1, 0 })] = 10.0f; // "barro" en la fila de arriba
		cost[g.index({ 1, 1 })] = 10.0f;
		const auto path = find_path(g, g.index({ 0, 0 }), g.index({ 2, 0 }), &cost);
		REQUIRE_FALSE(path.empty());
		for (int c : path) {
			CHECK(cost[c] == 0.0f); // va por abajo
		}
	}

	TEST_CASE("Funciona en un laberinto generado: base -> centro") {
		MazeGrid maze;
		MazeGrid::Options options;
		options.cells_w = 20;
		options.cells_h = 14;
		options.corridor_width = 3;
		options.vault_radius = 4;
		options.seed = 7;
		maze.generate(options);
		const int from = maze.cell_index_of(maze.base_cells(1)[0]);
		const int to = maze.cell_index_of(maze.center());
		const auto path = find_path(maze.passages(), from, to);
		REQUIRE_FALSE(path.empty());
		CHECK(is_valid_path(maze.passages(), path));
		// El centro de cada celda del camino es suelo.
		for (int c : path) {
			CHECK_FALSE(maze.is_wall(maze.cell_center(c)));
		}
	}

	TEST_CASE("cell_index_of: casillas de una celda y de su pared derecha") {
		MazeGrid maze;
		MazeGrid::Options options;
		options.cells_w = 5;
		options.cells_h = 5;
		options.corridor_width = 3; // celda 0: casillas 1..3, pared en 4
		options.seed = 1;
		maze.generate(options);
		CHECK(maze.cell_index_of({ 1, 1 }) == 0);
		CHECK(maze.cell_index_of({ 3, 3 }) == 0);
		CHECK(maze.cell_index_of({ 5, 1 }) == 1);
		CHECK(maze.cell_index_of({ 0, 0 }) == 0); // borde: se ajusta
		CHECK(maze.cell_index_of(maze.cell_center(7)) == 7);
	}

	TEST_CASE("Errores en cruces: 0% nunca se desvía, 100% siempre (si hay otra salida)") {
		const PassageGrid g = open_grid(3, 3);
		const int center = g.index({ 1, 1 }); // 4 salidas
		const int planned = g.index({ 2, 1 });
		const int previous = g.index({ 0, 1 });
		Rng rng(3);
		for (int i = 0; i < 50; i++) {
			CHECK(choose_next_cell(g, center, planned, previous, 0.0f, rng) == planned);
			const int other = choose_next_cell(g, center, planned, previous, 1.0f, rng);
			CHECK(other != planned);
			CHECK(other != previous);
			CHECK(g.is_open(center, other));
		}
		// En un pasillo (2 salidas) nunca se equivoca.
		PassageGrid line = open_grid(3, 1);
		CHECK(choose_next_cell(line, 1, 2, 0, 1.0f, rng) == 2);
	}
}

TEST_SUITE("Bots: cerebro (Utility AI)") {
	TEST_CASE("Sin trofeo disponible ni objetos: explora") {
		auto brain = BotBrain::make_default(no_noise());
		BotWorld w = world_at({ 5, 5 });
		w.trophy_available = false;
		Rng rng(1);
		const auto d = brain->evaluate(w, rng);
		CHECK(d.goal == BotGoal::Explore);
		CHECK(d.target == Cell{ 10, 10 });
	}

	TEST_CASE("Va por el trofeo y, al tenerlo, lo lleva a su base") {
		auto brain = BotBrain::make_default(no_noise());
		Rng rng(1);
		BotWorld w = world_at({ 5, 5 });
		CHECK(brain->update(0.016f, w, rng)); // primera decisión inmediata
		CHECK(brain->current().goal == BotGoal::GetTrophy);
		CHECK(brain->current().target == Cell{ 40, 30 });
		w.carrying_trophy = true;
		w.trophy_available = false;
		// Piensa (cada 0,2 s) y adopta la decisión nueva tras el tiempo de reacción (0,5 s).
		float t = 0.0f;
		bool changed = false;
		while (t < 2.0f && !changed) {
			changed = brain->update(0.05f, w, rng);
			t += 0.05f;
		}
		CHECK(changed);
		CHECK(brain->current().goal == BotGoal::DeliverTrophy);
		CHECK(brain->current().target == Cell{ 2, 2 }); // centro de la base
		CHECK(t >= 0.5f); // no antes del tiempo de reacción
	}

	TEST_CASE("Un objeto que no vio no le interesa; uno visto y cercano distrae según la dificultad") {
		Rng rng(1);
		BotWorld w = world_at({ 5, 5 });
		BotTuning distracted = no_noise();
		distracted.pickup_interest = 1.0f;
		auto easy = BotBrain::make_default(distracted);
		CHECK(easy->evaluate(w, rng).goal == BotGoal::GetTrophy); // no vio ninguno
		w.seen_pickups = { { 6, 5 } };
		CHECK(easy->evaluate(w, rng).goal == BotGoal::CollectPickup);
		BotTuning focused = no_noise();
		focused.pickup_interest = 0.2f;
		auto expert = BotBrain::make_default(focused);
		CHECK(expert->evaluate(w, rng).goal == BotGoal::GetTrophy);
	}

	TEST_CASE("Con el trofeo encima, entregar siempre gana a un objeto") {
		BotTuning t = no_noise();
		t.pickup_interest = 1.0f;
		auto brain = BotBrain::make_default(t);
		Rng rng(1);
		BotWorld w = world_at({ 5, 5 });
		w.carrying_trophy = true;
		w.trophy_available = false;
		w.seen_pickups = { { 5, 6 } };
		CHECK(brain->evaluate(w, rng).goal == BotGoal::DeliverTrophy);
	}

	TEST_CASE("Variación: misma semilla = mismas decisiones; sin variación no hay sorpresas") {
		BotTuning noisy = no_noise();
		noisy.variation = 0.5f;
		noisy.pickup_interest = 0.6f;
		BotWorld w = world_at({ 5, 5 });
		w.seen_pickups = { { 9, 5 } };
		auto a = BotBrain::make_default(noisy);
		auto b = BotBrain::make_default(noisy);
		Rng ra(42);
		Rng rb(42);
		bool saw_trophy = false;
		bool saw_pickup = false;
		for (int i = 0; i < 200; i++) {
			const auto da = a->evaluate(w, ra);
			CHECK(da.goal == b->evaluate(w, rb).goal);
			saw_trophy = saw_trophy || da.goal == BotGoal::GetTrophy;
			saw_pickup = saw_pickup || da.goal == BotGoal::CollectPickup;
		}
		CHECK(saw_trophy); // con mucha variación a veces elige una cosa...
		CHECK(saw_pickup); // ...y a veces otra ("a veces pro, a veces noob")
	}

	TEST_CASE("Acciones nuevas se agregan sin tocar el cerebro") {
		struct AlwaysExplore : lv::core::BotAction {
			BotGoal goal() const override { return BotGoal::Explore; }
			float score(const BotWorld &, const BotTuning &) const override { return 5.0f; }
			Cell target(const BotWorld &) const override { return { 99, 99 }; }
		};
		BotBrain brain(no_noise());
		Rng rng(1);
		BotWorld w = world_at({ 5, 5 });
		CHECK(brain.evaluate(w, rng).goal == BotGoal::None); // sin acciones no hace nada
		brain.add_action(std::make_unique<AlwaysExplore>());
		CHECK(brain.evaluate(w, rng).target == Cell{ 99, 99 });
	}
}
