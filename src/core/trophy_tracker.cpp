#include "core/trophy_tracker.h"

namespace lv::core {

void TrophyTracker::setup(const Cell &p_trophy_cell, const std::vector<Cell> &p_bases, int p_pickup_range, int p_base_radius) {
	state_ = State::InVault;
	trophy_cell_ = p_trophy_cell;
	bases_ = p_bases;
	carrier_ = NO_ONE;
	winner_ = NO_ONE;
	pickup_range_ = p_pickup_range < 0 ? 0 : p_pickup_range;
	base_radius_ = p_base_radius < 0 ? 0 : p_base_radius;
}

std::vector<TrophyTracker::Event> TrophyTracker::update(int p_participant, const Cell &p_cell) {
	std::vector<Event> events;
	if (!valid(p_participant)) {
		return events;
	}
	switch (state_) {
		case State::InVault:
		case State::Dropped:
			if (chebyshev_distance(p_cell, trophy_cell_) <= pickup_range_) {
				state_ = State::Carried;
				carrier_ = p_participant;
				events.push_back({ Event::Type::Taken, p_participant, p_cell });
			}
			break;
		case State::Carried:
			if (p_participant == carrier_ && is_in_own_base(p_participant, p_cell)) {
				state_ = State::Delivered;
				winner_ = p_participant;
				carrier_ = NO_ONE;
				events.push_back({ Event::Type::Delivered, p_participant, p_cell });
			}
			break;
		case State::Delivered:
			break; // la partida terminó
	}
	return events;
}

std::vector<TrophyTracker::Event> TrophyTracker::drop(const Cell &p_cell) {
	std::vector<Event> events;
	if (state_ != State::Carried) {
		return events;
	}
	const int previous = carrier_;
	state_ = State::Dropped;
	carrier_ = NO_ONE;
	trophy_cell_ = p_cell;
	events.push_back({ Event::Type::Dropped, previous, p_cell });
	return events;
}

bool TrophyTracker::relocate(const Cell &p_cell) {
	if (state_ != State::InVault && state_ != State::Dropped) {
		return false;
	}
	trophy_cell_ = p_cell;
	return true;
}

bool TrophyTracker::is_in_own_base(int p_participant, const Cell &p_cell) const {
	return valid(p_participant) && chebyshev_distance(p_cell, bases_[p_participant]) <= base_radius_;
}

} // namespace lv::core
