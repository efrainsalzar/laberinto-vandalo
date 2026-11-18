#include "core/weapon.h"

namespace lv::core {

float effective_range(const WeaponSpec &p_spec, float p_vision) {
	if (!p_spec.range_from_vision) {
		return p_spec.range;
	}
	const float range = p_vision * p_spec.range_vision_factor;
	return range > 0.5f ? range : 0.5f;
}

void WeaponState::setup(const WeaponSpec &p_spec) {
	spec_ = p_spec;
	state_ = State::Ready;
	timer_ = 0.0f;
	ammo_ = has_infinite_ammo() ? -1 : spec_.magazine_size;
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
	if (state_ != State::Ready) {
		return false;
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
