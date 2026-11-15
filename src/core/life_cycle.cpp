#include "core/life_cycle.h"

namespace lv::core {

void LifeCycle::setup(float p_respawn_delay, float p_protection) {
	respawn_delay_ = p_respawn_delay < 0.0f ? 0.0f : p_respawn_delay;
	protection_ = p_protection < 0.0f ? 0.0f : p_protection;
	state_ = State::Alive;
	timer_ = 0.0f;
}

bool LifeCycle::eliminate() {
	if (state_ != State::Alive) {
		return false;
	}
	state_ = State::Eliminated;
	timer_ = respawn_delay_;
	return true;
}

std::vector<LifeCycle::Event> LifeCycle::tick(float p_delta) {
	std::vector<Event> events;
	if (state_ == State::Alive) {
		return events;
	}
	timer_ -= p_delta;
	if (state_ == State::Eliminated && timer_ <= 0.0f) {
		const float extra = -timer_; // tiempo que sobró en este tramo
		state_ = State::Protected;
		timer_ = protection_ - extra;
		events.push_back(Event::Respawned);
	}
	if (state_ == State::Protected && timer_ <= 0.0f) {
		state_ = State::Alive;
		timer_ = 0.0f;
		events.push_back(Event::ProtectionEnded);
	}
	return events;
}

} // namespace lv::core
