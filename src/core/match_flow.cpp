#include "core/match_flow.h"

#include <algorithm>

namespace lv::core {

void MatchFlow::start(const Rules &p_rules) {
	rules_ = p_rules;
	started_ = true;
	phase_ = Phase::Preparation;
	outcome_ = Outcome::None;
	winner_ = -1;
	elapsed_ = 0.0f;

	float shares[3] = { std::max(0.0f, rules_.preparation_share), std::max(0.0f, rules_.assault_share),
		std::max(0.0f, rules_.heist_share) };
	float total = shares[0] + shares[1] + shares[2];
	if (total <= 0.0f) {
		shares[0] = 0.40f;
		shares[1] = 0.35f;
		shares[2] = 0.25f;
		total = 1.0f;
	}
	const float length = is_unlimited() ? std::max(1.0f, rules_.unlimited_reference) : rules_.duration;
	bounds_[0] = length * shares[0] / total;
	bounds_[1] = length * (shares[0] + shares[1]) / total;
	bounds_[2] = length; // sin límite: solo se usa como referencia (el Golpe no termina)
	warned_.assign(rules_.warnings.size(), false);
}

float MatchFlow::time_left() const {
	if (is_unlimited()) {
		return -1.0f;
	}
	return std::max(0.0f, rules_.duration - elapsed_);
}

float MatchFlow::phase_time_left() const {
	if (phase_ == Phase::Ended || (phase_ == Phase::Heist && is_unlimited())) {
		return -1.0f;
	}
	return std::max(0.0f, bounds_[static_cast<int>(phase_)] - elapsed_);
}

float MatchFlow::phase_start(Phase p_phase) const {
	switch (p_phase) {
		case Phase::Preparation:
			return 0.0f;
		case Phase::Assault:
			return bounds_[0];
		case Phase::Heist:
			return bounds_[1];
		case Phase::Ended:
			return bounds_[2];
	}
	return 0.0f;
}

std::vector<MatchFlow::Event> MatchFlow::advance(float p_delta) {
	std::vector<Event> events;
	if (!is_running() || p_delta <= 0.0f) {
		return events;
	}
	emit_until(elapsed_ + p_delta, events);
	return events;
}

std::vector<MatchFlow::Event> MatchFlow::declare_winner(int p_participant) {
	std::vector<Event> events;
	if (!is_running()) {
		return events;
	}
	phase_ = Phase::Ended;
	outcome_ = Outcome::Victory;
	winner_ = p_participant;
	Event e{ Event::Type::Ended };
	e.phase = Phase::Ended;
	e.seconds_left = time_left();
	e.outcome = Outcome::Victory;
	e.winner = p_participant;
	events.push_back(e);
	return events;
}

std::vector<MatchFlow::Event> MatchFlow::skip_phase() {
	std::vector<Event> events;
	if (!is_running()) {
		return events;
	}
	if (phase_ == Phase::Heist && is_unlimited()) {
		return events; // el Golpe sin límite no termina
	}
	emit_until(bounds_[static_cast<int>(phase_)], events);
	return events;
}

// Avanza el reloj hasta p_time emitiendo en orden avisos, cambios de fase y el fin.
void MatchFlow::emit_until(float p_time, std::vector<Event> &r_events) {
	while (is_running()) {
		// Próximo límite de fase (el Golpe sin límite no tiene).
		const int index = static_cast<int>(phase_);
		const bool has_bound = !(phase_ == Phase::Heist && is_unlimited());
		const float bound = has_bound ? bounds_[index] : p_time + 1.0f;
		const float until = std::min(p_time, bound);

		// Avisos que caen dentro de este tramo (solo con límite).
		if (!is_unlimited()) {
			// Del más lejano al más cercano al final, para que salgan en orden cronológico.
			std::vector<size_t> order;
			for (size_t i = 0; i < rules_.warnings.size(); i++) {
				order.push_back(i);
			}
			std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return rules_.warnings[a] > rules_.warnings[b]; });
			for (size_t i : order) {
				const float w = rules_.warnings[i];
				if (warned_[i] || w <= 0.0f || w >= rules_.duration) {
					continue;
				}
				if (rules_.duration - until <= w) {
					warned_[i] = true;
					Event e{ Event::Type::TimeWarning };
					e.phase = phase_;
					e.seconds_left = w;
					r_events.push_back(e);
				}
			}
		}

		elapsed_ = until;
		if (!has_bound || p_time < bound) {
			return;
		}
		// Se cruzó el límite de la fase.
		if (phase_ == Phase::Heist) {
			phase_ = Phase::Ended;
			outcome_ = Outcome::TimeUp;
			Event e{ Event::Type::Ended };
			e.phase = Phase::Ended;
			e.outcome = Outcome::TimeUp;
			r_events.push_back(e);
			return;
		}
		phase_ = static_cast<Phase>(index + 1);
		Event e{ Event::Type::PhaseChanged };
		e.phase = phase_;
		e.seconds_left = time_left();
		r_events.push_back(e);
		if (p_time <= bound) {
			return; // justo en el límite: la fase nueva empieza y no hay más tiempo que gastar
		}
	}
}

} // namespace lv::core
