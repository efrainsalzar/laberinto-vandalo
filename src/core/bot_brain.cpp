#include "core/bot_brain.h"

#include <algorithm>
#include <cmath>

namespace lv::core {

namespace {
float tile_distance(const Cell &p_a, const Cell &p_b) {
	const float dx = static_cast<float>(p_a.x - p_b.x);
	const float dy = static_cast<float>(p_a.y - p_b.y);
	return std::sqrt(dx * dx + dy * dy);
}

const Cell *nearest(const BotWorld &p_world) {
	const Cell *best = nullptr;
	float best_d = 0.0f;
	for (const Cell &c : p_world.seen_pickups) {
		const float d = tile_distance(p_world.self, c);
		if (best == nullptr || d < best_d) {
			best = &c;
			best_d = d;
		}
	}
	return best;
}
} // namespace

float GetTrophyAction::score(const BotWorld &p_world, const BotTuning &) const {
	return (!p_world.carrying_trophy && p_world.trophy_available) ? 0.8f : 0.0f;
}

float DeliverTrophyAction::score(const BotWorld &p_world, const BotTuning &) const {
	return p_world.carrying_trophy ? 1.0f : 0.0f;
}

float CollectPickupAction::score(const BotWorld &p_world, const BotTuning &p_tuning) const {
	const Cell *pickup = nearest(p_world);
	if (pickup == nullptr) {
		return 0.0f;
	}
	const float closeness = 1.0f - std::min(1.0f, tile_distance(p_world.self, *pickup) / RANGE_TILES);
	// Con el trofeo encima casi nada lo distrae.
	const float focus = p_world.carrying_trophy ? 0.3f : 1.0f;
	return std::clamp(p_tuning.pickup_interest, 0.0f, 1.0f) * closeness * focus;
}

Cell CollectPickupAction::target(const BotWorld &p_world) const {
	const Cell *pickup = nearest(p_world);
	return pickup != nullptr ? *pickup : p_world.self;
}

BotBrain::BotBrain(const BotTuning &p_tuning) :
		tuning_(p_tuning) {}

std::unique_ptr<BotBrain> BotBrain::make_default(const BotTuning &p_tuning) {
	auto brain = std::make_unique<BotBrain>(p_tuning);
	brain->add_action(std::make_unique<DeliverTrophyAction>());
	brain->add_action(std::make_unique<GetTrophyAction>());
	brain->add_action(std::make_unique<CollectPickupAction>());
	brain->add_action(std::make_unique<ExploreAction>());
	return brain;
}

void BotBrain::add_action(std::unique_ptr<BotAction> p_action) {
	if (p_action) {
		actions_.push_back(std::move(p_action));
	}
}

BotDecision BotBrain::evaluate(const BotWorld &p_world, Rng &r_rng) const {
	BotDecision best;
	bool found = false;
	for (const auto &action : actions_) {
		float score = action->score(p_world, tuning_);
		if (score <= 0.0f) {
			continue;
		}
		if (tuning_.variation > 0.0f) {
			score += (r_rng.unit() * 2.0f - 1.0f) * tuning_.variation;
		}
		if (action->goal() == current_.goal) {
			score += KEEP_BONUS;
		}
		if (!found || score > best.score) {
			best = { action->goal(), action->target(p_world), score };
			found = true;
		}
	}
	return best;
}

bool BotBrain::update(float p_delta, const BotWorld &p_world, Rng &r_rng) {
	if (has_pending_) {
		pending_timer_ -= p_delta;
		if (pending_timer_ <= 0.0f) {
			has_pending_ = false;
			current_ = pending_;
			return true;
		}
	}
	think_timer_ -= p_delta;
	if (think_timer_ > 0.0f) {
		return false;
	}
	think_timer_ = std::max(0.05f, tuning_.think_interval);
	const BotDecision decision = evaluate(p_world, r_rng);
	if (decision.goal == BotGoal::None) {
		return false;
	}
	if (current_.goal == BotGoal::None) {
		current_ = decision; // primera decisión: sin demora
		has_pending_ = false;
		return true;
	}
	if (decision.goal == current_.goal) {
		has_pending_ = false; // volvió a preferir lo que ya hacía
		if (decision.target != current_.target) {
			current_ = decision;
			return true;
		}
		current_.score = decision.score;
		return false;
	}
	if (!has_pending_ || pending_.goal != decision.goal) {
		pending_timer_ = std::max(0.0f, tuning_.reaction_time); // "se da cuenta" con retraso
	}
	pending_ = decision;
	has_pending_ = true;
	if (pending_timer_ <= 0.0f) {
		has_pending_ = false;
		current_ = pending_;
		return true;
	}
	return false;
}

} // namespace lv::core
