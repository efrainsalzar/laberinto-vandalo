#include "core/maze_shift.h"

#include "core/perf.h"

namespace lv::core {

std::vector<PassageChange> plan_maze_shift(const MazeGrid &p_grid, int p_pairs, Rng &r_rng) {
	PerfScope perf("maze.plan");
	std::vector<PassageChange> plan;
	const PassageGrid &passages = p_grid.passages();
	if (p_pairs <= 0 || passages.cell_count() < 2) {
		return plan;
	}

	// Muestreo al azar de pasajes en lugar de recorrer todo el mapa: el costo depende de cuántos
	// cambios se piden, no del tamaño del mapa.
	const int max_attempts = p_pairs * 80;
	auto random_edge = [&](int &r_a, int &r_b) -> bool {
		r_a = r_rng.range(passages.cell_count());
		const Cell c = passages.cell(r_a);
		const bool east = r_rng.chance(0.5f);
		if (east && c.x + 1 < passages.width()) {
			r_b = r_a + 1;
		} else if (!east && c.y + 1 < passages.height()) {
			r_b = r_a + passages.width();
		} else {
			return false;
		}
		return !p_grid.is_protected_passage(r_a, r_b);
	};

	// Se trabaja sobre una copia para que cada decisión tenga en cuenta las anteriores.
	PassageGrid scratch = passages;

	// 1) Aperturas: siempre seguras.
	int opened = 0;
	for (int attempt = 0; attempt < max_attempts && opened < p_pairs; attempt++) {
		int a = 0;
		int b = 0;
		if (random_edge(a, b) && !scratch.is_open(a, b)) {
			scratch.carve(a, b);
			plan.push_back({ a, b, true });
			opened++;
		}
	}

	// 2) Cierres: solo si queda un camino alternativo (búsqueda local acotada).
	int closed = 0;
	for (int attempt = 0; attempt < max_attempts && closed < opened; attempt++) {
		int a = 0;
		int b = 0;
		if (!random_edge(a, b) || !scratch.is_open(a, b)) {
			continue;
		}
		bool just_opened = false;
		for (const PassageChange &p : plan) {
			just_opened = just_opened || ((p.a == a && p.b == b) || (p.a == b && p.b == a));
		}
		if (just_opened || !scratch.has_alternative_path(a, b)) {
			continue;
		}
		scratch.close(a, b);
		plan.push_back({ a, b, false });
		closed++;
	}

	// 3) Equilibrio: si se encontraron menos cierres que aperturas, se descartan aperturas sobrantes
	//    (las más recientes), pero solo si quitarlas no desconecta nada (se validan como un cierre).
	for (int i = static_cast<int>(plan.size()) - 1; i >= 0 && closed < opened; i--) {
		const PassageChange p = plan[i];
		if (!p.open || !scratch.has_alternative_path(p.a, p.b)) {
			continue;
		}
		scratch.close(p.a, p.b);
		plan.erase(plan.begin() + i);
		opened--;
	}
	return plan;
}

} // namespace lv::core
