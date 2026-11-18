#include "core/weapon.h"

#include <cmath>

namespace lv::core {

float effective_range(const WeaponSpec &p_spec, float p_vision) {
	if (!p_spec.range_from_vision) {
		return p_spec.range;
	}
	const float range = p_vision * p_spec.range_vision_factor;
	return range > 0.5f ? range : 0.5f;
}

void spread_directions(float p_dir_x, float p_dir_y, int p_pellets, float p_spread_deg, std::vector<Direction> &r_out) {
	r_out.clear();
	const float len = std::sqrt(p_dir_x * p_dir_x + p_dir_y * p_dir_y);
	if (len <= 0.0f) {
		return;
	}
	const float base = std::atan2(p_dir_y, p_dir_x);
	const int count = p_pellets < 1 ? 1 : p_pellets;
	const float spread = (count > 1 ? p_spread_deg : 0.0f) * 3.14159265f / 180.0f;
	for (int i = 0; i < count; i++) {
		const float k = count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1) - 0.5f : 0.0f;
		const float angle = base + k * spread;
		r_out.push_back({ std::cos(angle), std::sin(angle) });
	}
}

void WeaponState::setup(const WeaponSpec &p_spec) {
	spec_ = p_spec;
	state_ = State::Ready;
	timer_ = 0.0f;
	ammo_ = has_infinite_ammo() ? -1 : spec_.magazine_size;
	total_ammo_ = spec_.ammo > 0 ? spec_.ammo : -1;
	energy_ = spec_.energy > 0.0f ? spec_.energy : -1.0f;
}

bool WeaponState::drain(float p_delta) {
	if (is_depleted()) {
		return false;
	}
	if (energy_ > 0.0f) {
		energy_ -= p_delta;
		if (energy_ <= 0.0f) {
			energy_ = 0.0f; // el último tramo sí dispara; desde el siguiente frame queda vacía
		}
	}
	return true;
}

Trigger next_trigger(bool p_was_down, bool p_is_down) {
	if (p_is_down) {
		return p_was_down ? Trigger::Held : Trigger::Pressed;
	}
	return p_was_down ? Trigger::Released : Trigger::Idle;
}

void WeaponState::tick(float p_delta) {
	if (state_ == State::Ready) {
		return;
	}
	timer_ -= p_delta;
	if (timer_ > 0.0f) {
		return;
	}
	timer_ = 0.0f;
	if (state_ == State::Reloading) {
		ammo_ = spec_.magazine_size;
	}
	state_ = State::Ready;
}

bool WeaponState::try_fire() {
	if (state_ != State::Ready || is_depleted()) {
		return false;
	}
	if (total_ammo_ > 0) {
		total_ammo_--;
	}
	if (!has_infinite_ammo()) {
		ammo_--;
		if (ammo_ <= 0) {
			ammo_ = 0;
			state_ = State::Reloading;
			timer_ = spec_.reload_time > spec_.fire_interval ? spec_.reload_time : spec_.fire_interval;
			return true;
		}
	}
	state_ = State::Cooling;
	timer_ = spec_.fire_interval;
	if (timer_ <= 0.0f) {
		state_ = State::Ready; // sin cadencia: puede volver a disparar en el siguiente frame
		timer_ = 0.0f;
	}
	return true;
}

} // namespace lv::core
