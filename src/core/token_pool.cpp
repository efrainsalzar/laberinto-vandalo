#include "core/token_pool.h"

#include <algorithm>

namespace lv::core {

const std::vector<int> &TokenPool::assign(const std::vector<Candidate> &p_candidates) {
	struct Scored {
		int id;
		float score; // distancia efectiva (menor = mejor)
	};
	std::vector<Scored> scored;
	scored.reserve(p_candidates.size());
	for (const Candidate &c : p_candidates) {
		scored.push_back({ c.id, has(c.id) ? c.distance * keep_factor_ : c.distance });
	}
	// Orden estable con desempate por id: mismo resultado en todas las plataformas.
	std::sort(scored.begin(), scored.end(), [](const Scored &a, const Scored &b) {
		return a.score != b.score ? a.score < b.score : a.id < b.id;
	});
	holders_.clear();
	for (size_t i = 0; i < scored.size() && static_cast<int>(holders_.size()) < max_; i++) {
		holders_.push_back(scored[i].id);
	}
	return holders_;
}

bool TokenPool::has(int p_id) const {
	return std::find(holders_.begin(), holders_.end(), p_id) != holders_.end();
}

} // namespace lv::core
