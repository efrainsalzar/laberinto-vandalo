#include "core/bot_gunner.h"

#include <cmath>

namespace lv::core {

void BotGunner::setup(const Tuning &p_tuning) {
	tuning_ = p_tuning;
	reset();
}

void BotGunner::reset() {
	target_ = -1;
	wait_ = 0.0f;
	error_timer_ = 0.0f;
}

GunDecision BotGunner::update(float p_delta, float p_self_x, float p_self_y, const std::vector<GunTarget> &p_targets, Rng &r_rng) {
	GunDecision decision;
	const GunTarget *best = nullptr;
	float best_d2 = tuning_.max_range * tuning_.max_range;
	for (const GunTarget &t : p_targets) {
		const float dx = t.x - p_self_x;
		const float dy = t.y - p_self_y;
		const float d2 = dx * dx + dy * dy;
		if (d2 <= best_d2) { // con empate gana el primero (orden fijo)
			if (best == nullptr || d2 < best_d2) {
				best = &t;
				best_d2 = d2;
			}
		}
	}
	if (best == nullptr) {
		reset();
		return decision;
	}
	if (best->id != target_) {
		target_ = best->id;
		wait_ = tuning_.reaction_time; // blanco nuevo: tarda en reaccionar
	}
	const float dx = best->x - p_self_x;
	const float dy = best->y - p_self_y;
	decision.target = target_;
	decision.aim_x = dx;
	decision.aim_y = dy;
	if (wait_ > 0.0f) {
		wait_ -= p_delta;
		return decision; // ya apunta, pero todavía no dispara
	}
	// Desvío al azar en [-error, +error]; se mantiene error_hold segundos.
	error_timer_ -= p_delta;
	if (error_timer_ <= 0.0f) {
		const float error = tuning_.aim_error_deg * 3.14159265f / 180.0f;
		error_angle_ = (r_rng.unit() * 2.0f - 1.0f) * error;
		error_timer_ = tuning_.error_hold;
	}
	const float c = std::cos(error_angle_);
	const float s = std::sin(error_angle_);
	decision.aim_x = dx * c - dy * s;
	decision.aim_y = dx * s + dy * c;
	decision.fire = true;
	return decision;
}

} // namespace lv::core
