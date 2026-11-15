#include "core/stats.h"

#include <algorithm>
#include <limits>

namespace lv::core {

namespace {
size_t idx(StatType p_stat) {
	return static_cast<size_t>(p_stat);
}
} // namespace

Stats::Stats() {
	min_.fill(0.0f);
	max_.fill(std::numeric_limits<float>::max());
}

void Stats::set_base(StatType p_stat, float p_value) {
	base_[idx(p_stat)] = p_value;
}

float Stats::get_base(StatType p_stat) const {
	return base_[idx(p_stat)];
}

void Stats::set_limits(StatType p_stat, float p_min, float p_max) {
	min_[idx(p_stat)] = p_min;
	max_[idx(p_stat)] = std::max(p_min, p_max);
}

float Stats::get(StatType p_stat) const {
	float sum = base_[idx(p_stat)];
	float factor = 1.0f;
	for (const ActiveModifier &active : modifiers_) {
		if (active.modifier.stat != p_stat) {
			continue;
		}
		if (active.modifier.op == ModifierOp::Add) {
			sum += active.modifier.value;
		} else {
			factor *= active.modifier.value;
		}
	}
	return std::clamp(sum * factor, min_[idx(p_stat)], max_[idx(p_stat)]);
}

int Stats::add_modifier(const StatModifier &p_modifier) {
	const int id = next_id_++;
	modifiers_.push_back({ id, p_modifier, p_modifier.duration });
	return id;
}

bool Stats::remove_modifier(int p_id) {
	const auto it = std::find_if(modifiers_.begin(), modifiers_.end(), [p_id](const ActiveModifier &p_m) { return p_m.id == p_id; });
	if (it == modifiers_.end()) {
		return false;
	}
	modifiers_.erase(it);
	return true;
}

int Stats::remove_modifiers_from(const std::string &p_source) {
	const size_t before = modifiers_.size();
	modifiers_.erase(std::remove_if(modifiers_.begin(), modifiers_.end(), [&p_source](const ActiveModifier &p_m) { return p_m.modifier.source == p_source; }), modifiers_.end());
	return static_cast<int>(before - modifiers_.size());
}

std::vector<StatType> Stats::tick(double p_delta) {
	std::vector<StatType> changed;
	for (auto it = modifiers_.begin(); it != modifiers_.end();) {
		if (it->remaining == StatModifier::PERMANENT) {
			++it;
			continue;
		}
		it->remaining -= p_delta;
		if (it->remaining > 0.0) {
			++it;
			continue;
		}
		if (std::find(changed.begin(), changed.end(), it->modifier.stat) == changed.end()) {
			changed.push_back(it->modifier.stat);
		}
		it = modifiers_.erase(it);
	}
	return changed;
}

} // namespace lv::core
