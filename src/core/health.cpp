#include "core/health.h"

namespace lv::core {

void Health::setup(float p_max) {
	max_ = p_max > 0.0f ? p_max : 1.0f;
	current_ = max_;
}

DamageResult Health::damage(const DamageInfo &p_info) {
	DamageResult result;
	if (p_info.amount <= 0.0f || is_dead()) {
		return result;
	}
	result.applied = p_info.amount < current_ ? p_info.amount : current_;
	current_ -= result.applied;
	if (current_ <= 0.0f) {
		current_ = 0.0f;
		result.killed = true;
	}
	return result;
}

float Health::heal(float p_amount) {
	if (p_amount <= 0.0f || is_dead()) {
		return 0.0f;
	}
	const float room = max_ - current_;
	const float healed = p_amount < room ? p_amount : room;
	current_ += healed;
	return healed;
}

float explosion_damage(float p_center, float p_edge, float p_radius, float p_distance) {
	if (p_radius <= 0.0f) {
		return p_center;
	}
	float t = p_distance / p_radius;
	t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	return p_center + (p_edge - p_center) * t;
}

void DamageBatch::add(float p_amount) {
	if (p_amount <= 0.0f) {
		return;
	}
	if (pending_ <= 0.0f) {
		timer_ = 0.0f;
	}
	pending_ += p_amount;
}

float DamageBatch::tick(float p_delta) {
	if (pending_ <= 0.0f) {
		return 0.0f;
	}
	timer_ += p_delta;
	if (timer_ < interval_) {
		return 0.0f;
	}
	return flush();
}

float DamageBatch::flush() {
	const float total = pending_;
	pending_ = 0.0f;
	timer_ = 0.0f;
	return total;
}

} // namespace lv::core
