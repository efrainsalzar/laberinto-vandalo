#pragma once

#include <vector>

namespace lv::core {

// Fichas de ataque ("attack tokens"): técnica de IA de juegos de acción (Batman Arkham, Doom) para que no
// ataquen todos a la vez. Hay `max` fichas; las reciben los candidatos más cercanos al objetivo y solo quien
// tiene ficha ataca (los demás hacen otra cosa: emboscar, juntar objetos...).
//
// - Inercia: quien ya tenía ficha compite con su distancia × `keep_factor` (< 1), así las fichas no saltan
//   de un bot a otro por diferencias mínimas.
// - Genérico: sirve para perseguir al portador del trofeo y, luego, para vándalos o guardianes.
// Ver docs/ARQUITECTURA.md §19.
class TokenPool {
public:
	struct Candidate {
		int id = -1;
		float distance = 0.0f;
	};

	explicit TokenPool(int p_max = 2, float p_keep_factor = 0.8f) :
			max_(p_max < 0 ? 0 : p_max), keep_factor_(p_keep_factor) {}

	void set_max(int p_max) { max_ = p_max < 0 ? 0 : p_max; }
	int max() const { return max_; }

	// Reparte las fichas entre los candidatos (los que no estén en la lista las pierden).
	// Devuelve los ids con ficha, del más cercano al más lejano.
	const std::vector<int> &assign(const std::vector<Candidate> &p_candidates);
	bool has(int p_id) const;
	const std::vector<int> &holders() const { return holders_; }
	void clear() { holders_.clear(); }

private:
	int max_;
	float keep_factor_;
	std::vector<int> holders_;
};

} // namespace lv::core
