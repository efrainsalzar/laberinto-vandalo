#include "core/maze_shift.h"

namespace lv::core {

std::vector<PassageChange> plan_maze_shift(const MazeGrid &p_grid, int p_pairs, Rng &r_rng) {
	std::vector<PassageChange> plan;
	if (p_pairs <= 0) {
		return plan;
	}
	const PassageGrid &passages = p_grid.passages();

	// Candidatos: pasajes no protegidos, abiertos (para cerrar) o cerrados (para abrir).
	std::vector<PassageChange> openable;
	std::vector<PassageChange> closable;
	for (int i = 0; i < passages.cell_count(); i++) {
		int n[4];
		const int count = passages.neighbors(i, n);
		for (int k = 0; k < count; k++) {
			if (n[k] < i || p_grid.is_protected_passage(i, n[k])) {
				continue;
			}
			if (passages.is_open(i, n[k])) {
				closable.push_back({ i, n[k], false });
			} else {
				openable.push_back({ i, n[k], true });
			}
		}
	}
	r_rng.shuffle(openable);
	r_rng.shuffle(closable);

	// 1) Aperturas: siempre seguras.
	PassageGrid scratch = passages;
	int opened = 0;
	for (const PassageChange &change : openable) {
		if (opened >= p_pairs) {
			break;
		}
		scratch.carve(change.a, change.b);
		plan.push_back(change);
		opened++;
	}

	// 2) Cierres: solo si, con todo lo planificado hasta ahora, el laberinto sigue conectado.
	int closed = 0;
	for (const PassageChange &change : closable) {
		if (closed >= opened) {
			break;
		}
		scratch.close(change.a, change.b);
		if (scratch.is_connected()) {
			plan.push_back(change);
			closed++;
		} else {
			scratch.carve(change.a, change.b); // deshacer: ese pasaje era imprescindible
		}
	}
	return plan;
}

} // namespace lv::core
