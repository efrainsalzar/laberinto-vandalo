#include "game/bot_controller.h"

#include "core/nav_path.h"
#include "core/perf.h"
#include "game/bot_coordinator.h"
#include "game/maze.h"
#include "game/participant.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

#include <algorithm>
#include <cmath>

// --- BotProfile -------------------------------------------------------------------------------

void BotProfile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_display_name", "value"), &BotProfile::set_display_name);
	ClassDB::bind_method(D_METHOD("get_display_name"), &BotProfile::get_display_name);
	ClassDB::bind_method(D_METHOD("set_reaction_time", "value"), &BotProfile::set_reaction_time);
	ClassDB::bind_method(D_METHOD("get_reaction_time"), &BotProfile::get_reaction_time);
	ClassDB::bind_method(D_METHOD("set_think_interval", "value"), &BotProfile::set_think_interval);
	ClassDB::bind_method(D_METHOD("get_think_interval"), &BotProfile::get_think_interval);
	ClassDB::bind_method(D_METHOD("set_mistake_chance", "value"), &BotProfile::set_mistake_chance);
	ClassDB::bind_method(D_METHOD("get_mistake_chance"), &BotProfile::get_mistake_chance);
	ClassDB::bind_method(D_METHOD("set_pickup_interest", "value"), &BotProfile::set_pickup_interest);
	ClassDB::bind_method(D_METHOD("get_pickup_interest"), &BotProfile::get_pickup_interest);
	ClassDB::bind_method(D_METHOD("set_variation", "value"), &BotProfile::set_variation);
	ClassDB::bind_method(D_METHOD("get_variation"), &BotProfile::get_variation);
	ClassDB::bind_method(D_METHOD("set_aim_error_deg", "value"), &BotProfile::set_aim_error_deg);
	ClassDB::bind_method(D_METHOD("get_aim_error_deg"), &BotProfile::get_aim_error_deg);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reaction_time", PROPERTY_HINT_RANGE, "0,3,0.05,suffix:s"), "set_reaction_time", "get_reaction_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "think_interval", PROPERTY_HINT_RANGE, "0.05,3,0.05,suffix:s"), "set_think_interval", "get_think_interval");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mistake_chance", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_mistake_chance", "get_mistake_chance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pickup_interest", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_pickup_interest", "get_pickup_interest");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "variation", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_variation", "get_variation");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "aim_error_deg", PROPERTY_HINT_RANGE, "0,45,0.5,suffix:grados"), "set_aim_error_deg", "get_aim_error_deg");
}

// --- BotController ----------------------------------------------------------------------------

void BotController::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_profile"), &BotController::get_profile);
	ClassDB::bind_method(D_METHOD("get_goal"), &BotController::get_goal);
	ClassDB::bind_method(D_METHOD("has_path"), &BotController::has_path);
}

void BotController::setup(BotCoordinator *p_coordinator, const Ref<BotProfile> &p_profile, const Rect2i &p_base_zone, uint32_t p_seed) {
	coordinator_ = p_coordinator;
	profile_ = p_profile;
	const lv::core::BotTuning tuning = p_profile.is_valid() ? p_profile->tuning() : lv::core::BotTuning();
	brain_ = lv::core::BotBrain::make_default(tuning);
	rng_ = std::make_unique<lv::core::Rng>(p_seed);
	// Desfase: no piensan todos los bots en el mismo frame.
	brain_->set_think_offset(rng_->unit() * tuning.think_interval);
	lv::core::BotGunner::Tuning gun;
	gun.reaction_time = tuning.reaction_time;
	gun.aim_error_deg = p_profile.is_valid() ? p_profile->get_aim_error_deg() : 12.0f;
	gunner_.setup(gun);
	seen_rivals_.clear();
	aim_scan_timer_ = rng_->unit() * AIM_SCAN_INTERVAL; // desfase entre bots
	base_ = { p_base_zone.position.x, p_base_zone.position.y, p_base_zone.size.x, p_base_zone.size.y };
	path_.clear();
	path_requested_ = false;
	world_ready_ = false;
	has_explore_target_ = false;
}

int BotController::get_goal() const {
	return brain_ ? static_cast<int>(brain_->current().goal) : 0;
}

lv::core::ParticipantCommand BotController::compute_command(const Participant &p_participant, double p_delta) {
	if (coordinator_ == nullptr || !brain_ || coordinator_->get_maze() == nullptr) {
		return {};
	}
	const float dt = static_cast<float>(p_delta);
	lv::core::ParticipantCommand command = compute_move(p_participant, dt);
	aim(p_participant, dt, command);
	return command;
}

// Gatillo (BotGunner): a quién ve se revisa cada AIM_SCAN_INTERVAL; la puntería sigue la posición actual.
void BotController::aim(const Participant &p_participant, float p_delta, lv::core::ParticipantCommand &r_command) {
	const WeaponComponent *weapon = p_participant.get_weapon();
	if (weapon == nullptr || weapon->get_active_weapon().is_null()) {
		return;
	}
	lv::core::PerfScope perf("bots.aim");
	aim_scan_timer_ -= p_delta;
	if (aim_scan_timer_ <= 0.0f) {
		aim_scan_timer_ = AIM_SCAN_INTERVAL;
		coordinator_->collect_visible_rivals(p_participant, seen_rivals_);
	}
	const Maze *maze = coordinator_->get_maze();
	gun_targets_.clear();
	for (const ObjectID id : seen_rivals_) {
		const Participant *rival = Object::cast_to<Participant>(ObjectDB::get_instance(id));
		if (rival == nullptr || !rival->is_vulnerable()) {
			continue;
		}
		const lv::core::GridPoint at = maze->world_to_grid_point(rival->get_global_position());
		gun_targets_.push_back({ rival->get_participant_id(), at.x, at.y });
	}
	gunner_.set_max_range(weapon->get_effective_range(&p_participant));
	const lv::core::GridPoint self = maze->world_to_grid_point(p_participant.get_global_position());
	const lv::core::GunDecision decision = gunner_.update(p_delta, self.x, self.y, gun_targets_, *rng_);
	r_command.fire = decision.fire;
	r_command.aim_x = decision.aim_x;
	r_command.aim_y = decision.aim_y;
}

lv::core::ParticipantCommand BotController::compute_move(const Participant &p_participant, float p_delta) {
	lv::core::PerfScope perf("bots.think");
	const float dt = p_delta;
	if (!has_explore_target_) {
		pick_explore_target();
	}
	// Percepción solo cuando va a pensar (armarla cuesta más que moverse).
	if (!world_ready_ || brain_->will_think(dt)) {
		coordinator_->build_world(p_participant, world_);
		world_.base = base_;
		world_.explore_target = explore_target_;
		world_ready_ = true;
	}
	if (brain_->update(dt, world_, *rng_)) {
		path_.clear();
		replan_at_end_ = false;
		request_path(p_participant);
	}
	if (path_.empty()) {
		retry_timer_ -= dt;
		if (!path_requested_ && retry_timer_ <= 0.0f && brain_->current().goal != lv::core::BotGoal::None) {
			request_path(p_participant); // sin camino (el mapa cambió o no había ruta): reintenta
		}
		return {};
	}

	const Vector2 position = p_participant.get_global_position();
	const StatsComponent *stats = p_participant.get_stats();
	const float speed = stats != nullptr ? stats->get_speed() : 0.0f;
	const float step = std::max(0.001f, speed * dt);

	Vector2 to = waypoint(path_index_) - position;
	float distance = to.length();
	if (distance <= std::max(3.0f, step)) {
		advance_waypoint(p_participant);
		if (path_.empty()) {
			return {}; // llegó
		}
		to = waypoint(path_index_) - position;
		distance = to.length();
		best_distance_ = distance;
		stuck_timer_ = 0.0f;
	}

	// Trabado: si no se acerca a su punto en STUCK_SECONDS, replantea.
	if (distance < best_distance_ - 2.0f) {
		best_distance_ = distance;
		stuck_timer_ = 0.0f;
	} else {
		stuck_timer_ += dt;
		if (stuck_timer_ > STUCK_SECONDS) {
			stuck_timer_ = 0.0f;
			path_.clear();
			brain_->think_soon();
			request_path(p_participant);
			return {};
		}
	}

	lv::core::ParticipantCommand command;
	if (distance > 0.0f) {
		const Vector2 direction = to / distance * std::min(1.0f, distance / step); // sin pasarse
		command.move_x = direction.x;
		command.move_y = direction.y;
	}
	return command;
}

void BotController::request_path(const Participant &p_participant) {
	if (path_requested_ || coordinator_ == nullptr || brain_->current().goal == lv::core::BotGoal::None) {
		return;
	}
	path_requested_ = true;
	Maze *maze = coordinator_->get_maze();
	const lv::core::Cell target = brain_->current().target;
	requested_target_ = target;
	coordinator_->request_path(Ref<BotController>(this), maze->world_to_cell(p_participant.get_global_position()), Vector2i(target.x, target.y));
	// Para saltar el centro de la celda actual si ya está alineado con el pasillo (ver apply_path).
	last_position_ = p_participant.get_global_position();
	last_radius_ = p_participant.get_body_radius();
}

void BotController::on_respawned(const Participant &) {
	generation_++;
	path_.clear();
	path_requested_ = false;
	replan_at_end_ = false;
	stuck_timer_ = 0.0f;
	world_ready_ = false;
	gunner_.reset();
	seen_rivals_.clear();
	if (brain_) {
		brain_->think_soon();
	}
}

void BotController::apply_path(const std::vector<int> &p_cells, const Vector2 &p_target_world, int p_generation) {
	if (p_generation != generation_) {
		return; // pedido de antes de ser eliminado: ya no sirve (y ya puede haber otro pedido en curso)
	}
	path_requested_ = false;
	if (brain_ && brain_->current().target != requested_target_) {
		path_.clear(); // el objetivo cambió mientras esperaba: compute_command pedirá otro
		return;
	}
	path_ = p_cells;
	if (path_.empty()) {
		retry_timer_ = 0.5f; // no hay ruta ahora (p. ej. mapa cambiando): reintenta en un rato
		brain_->think_soon();
		return;
	}
	target_world_ = p_target_world;
	path_index_ = 0;
	previous_cell_ = -1;
	stuck_timer_ = 0.0f;
	best_distance_ = 1e9f;
	if (path_.size() == 1) {
		path_index_ = 1; // destino en la misma celda: directo
	} else if (path_.size() >= 2 && coordinator_ != nullptr) {
		// Si ya está dentro del "carril" hacia la celda siguiente, no retrocede al centro de la actual.
		const Maze *maze = coordinator_->get_maze();
		const Vector2 a = waypoint(0);
		const Vector2 b = waypoint(1);
		const float half_lane = maze->get_grid().corridor_width() * maze->get_tile_px() / 2.0f - last_radius_ - 1.0f;
		const float offset = std::abs(a.x - b.x) < 0.5f ? std::abs(last_position_.x - a.x) : std::abs(last_position_.y - a.y);
		if (offset <= half_lane) {
			previous_cell_ = path_[0];
			path_index_ = 1;
		}
	}
}

void BotController::validate_path() {
	if (path_.empty() || coordinator_ == nullptr) {
		return;
	}
	const lv::core::PassageGrid &grid = coordinator_->get_maze()->get_grid().passages();
	const size_t from = path_index_ > 0 ? path_index_ - 1 : 0;
	for (size_t i = from + 1; i < path_.size(); i++) {
		if (!grid.is_open(path_[i - 1], path_[i])) {
			path_.clear(); // compute_command pedirá otro
			return;
		}
	}
}

Vector2 BotController::waypoint(size_t p_index) const {
	if (p_index >= path_.size()) {
		return target_world_;
	}
	const Maze *maze = coordinator_->get_maze();
	const lv::core::Cell c = maze->get_grid().cell_center(path_[p_index]);
	return maze->cell_to_world(Vector2i(c.x, c.y));
}

void BotController::advance_waypoint(const Participant &p_participant) {
	if (path_index_ >= path_.size()) {
		// Llegó al destino final.
		path_.clear();
		brain_->think_soon();
		if (brain_->current().goal == lv::core::BotGoal::Explore) {
			pick_explore_target();
			world_.explore_target = explore_target_;
		}
		return;
	}
	const int cell = path_[path_index_];
	if (path_index_ + 1 < path_.size()) {
		// ¿Se equivoca en este cruce? (dificultad)
		const lv::core::PassageGrid &grid = coordinator_->get_maze()->get_grid().passages();
		const int planned = path_[path_index_ + 1];
		const float mistake = profile_.is_valid() ? profile_->get_mistake_chance() : 0.0f;
		const int chosen = lv::core::choose_next_cell(grid, cell, planned, previous_cell_, mistake, *rng_);
		if (chosen != planned) {
			path_ = { cell, chosen };
			path_index_ = 1;
			previous_cell_ = cell;
			target_world_ = waypoint(1);
			replan_at_end_ = true;
			return;
		}
	} else if (replan_at_end_) {
		// Terminó el desvío: vuelve a planear desde aquí.
		replan_at_end_ = false;
		path_.clear();
		request_path(p_participant);
		return;
	}
	previous_cell_ = cell;
	path_index_++;
}

void BotController::pick_explore_target() {
	if (coordinator_ == nullptr || coordinator_->get_maze() == nullptr) {
		return;
	}
	const lv::core::MazeGrid &grid = coordinator_->get_maze()->get_grid();
	const int count = grid.passages().cell_count();
	if (count <= 0) {
		return;
	}
	explore_target_ = grid.cell_center(rng_->range(count));
	has_explore_target_ = true;
}
