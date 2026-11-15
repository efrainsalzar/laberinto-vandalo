#include "game/match_controller.h"

#include "core/perf.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

// --- MatchRules -----------------------------------------------------------------------------

void MatchRules::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_preparation_share", "value"), &MatchRules::set_preparation_share);
	ClassDB::bind_method(D_METHOD("get_preparation_share"), &MatchRules::get_preparation_share);
	ClassDB::bind_method(D_METHOD("set_assault_share", "value"), &MatchRules::set_assault_share);
	ClassDB::bind_method(D_METHOD("get_assault_share"), &MatchRules::get_assault_share);
	ClassDB::bind_method(D_METHOD("set_heist_share", "value"), &MatchRules::set_heist_share);
	ClassDB::bind_method(D_METHOD("get_heist_share"), &MatchRules::get_heist_share);
	ClassDB::bind_method(D_METHOD("set_warnings", "value"), &MatchRules::set_warnings);
	ClassDB::bind_method(D_METHOD("get_warnings"), &MatchRules::get_warnings);
	ClassDB::bind_method(D_METHOD("set_preparation_maze_speed", "value"), &MatchRules::set_preparation_maze_speed);
	ClassDB::bind_method(D_METHOD("get_preparation_maze_speed"), &MatchRules::get_preparation_maze_speed);
	ClassDB::bind_method(D_METHOD("set_assault_maze_speed", "value"), &MatchRules::set_assault_maze_speed);
	ClassDB::bind_method(D_METHOD("get_assault_maze_speed"), &MatchRules::get_assault_maze_speed);
	ClassDB::bind_method(D_METHOD("set_heist_maze_speed", "value"), &MatchRules::set_heist_maze_speed);
	ClassDB::bind_method(D_METHOD("get_heist_maze_speed"), &MatchRules::get_heist_maze_speed);

	ADD_GROUP("Parte de cada fase", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "preparation_share", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_preparation_share", "get_preparation_share");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "assault_share", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_assault_share", "get_assault_share");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "heist_share", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_heist_share", "get_heist_share");
	ADD_GROUP("Avisos", "");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "warnings"), "set_warnings", "get_warnings");
	ADD_GROUP("Ritmo del laberinto vivo", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "preparation_maze_speed", PROPERTY_HINT_RANGE, "0,5,0.1"), "set_preparation_maze_speed", "get_preparation_maze_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "assault_maze_speed", PROPERTY_HINT_RANGE, "0,5,0.1"), "set_assault_maze_speed", "get_assault_maze_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "heist_maze_speed", PROPERTY_HINT_RANGE, "0,5,0.1"), "set_heist_maze_speed", "get_heist_maze_speed");
}

lv::core::MatchFlow::Rules MatchRules::to_rules(float p_duration) const {
	lv::core::MatchFlow::Rules rules;
	rules.duration = p_duration;
	rules.preparation_share = preparation_share_;
	rules.assault_share = assault_share_;
	rules.heist_share = heist_share_;
	rules.warnings.clear();
	for (int i = 0; i < warnings_.size(); i++) {
		rules.warnings.push_back(warnings_[i]);
	}
	return rules;
}

// --- MatchController ------------------------------------------------------------------------

void MatchController::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start", "duration"), &MatchController::start);
	ClassDB::bind_method(D_METHOD("stop"), &MatchController::stop);
	ClassDB::bind_method(D_METHOD("declare_winner", "participant"), &MatchController::declare_winner);
	ClassDB::bind_method(D_METHOD("skip_phase"), &MatchController::skip_phase);
	ClassDB::bind_method(D_METHOD("get_phase"), &MatchController::get_phase);
	ClassDB::bind_method(D_METHOD("get_outcome"), &MatchController::get_outcome);
	ClassDB::bind_method(D_METHOD("get_winner"), &MatchController::get_winner);
	ClassDB::bind_method(D_METHOD("is_running"), &MatchController::is_running);
	ClassDB::bind_method(D_METHOD("is_unlimited"), &MatchController::is_unlimited);
	ClassDB::bind_method(D_METHOD("get_elapsed"), &MatchController::get_elapsed);
	ClassDB::bind_method(D_METHOD("get_time_left"), &MatchController::get_time_left);
	ClassDB::bind_method(D_METHOD("get_phase_time_left"), &MatchController::get_phase_time_left);
	ClassDB::bind_method(D_METHOD("get_maze_speed"), &MatchController::get_maze_speed);
	ClassDB::bind_static_method("MatchController", D_METHOD("get_phase_name", "phase"), &MatchController::get_phase_name);
	ClassDB::bind_method(D_METHOD("set_rules", "value"), &MatchController::set_rules);
	ClassDB::bind_method(D_METHOD("get_rules"), &MatchController::get_rules);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "rules", PROPERTY_HINT_RESOURCE_TYPE, "MatchRules"), "set_rules", "get_rules");

	BIND_ENUM_CONSTANT(PHASE_PREPARATION);
	BIND_ENUM_CONSTANT(PHASE_ASSAULT);
	BIND_ENUM_CONSTANT(PHASE_HEIST);
	BIND_ENUM_CONSTANT(PHASE_ENDED);
	BIND_ENUM_CONSTANT(OUTCOME_NONE);
	BIND_ENUM_CONSTANT(OUTCOME_VICTORY);
	BIND_ENUM_CONSTANT(OUTCOME_TIME_UP);

	ADD_SIGNAL(MethodInfo("phase_changed", PropertyInfo(Variant::INT, "phase"), PropertyInfo(Variant::STRING, "phase_name")));
	ADD_SIGNAL(MethodInfo("time_warning", PropertyInfo(Variant::FLOAT, "seconds_left")));
	ADD_SIGNAL(MethodInfo("match_ended", PropertyInfo(Variant::INT, "outcome"), PropertyInfo(Variant::OBJECT, "winner")));
}

MatchController::MatchController() {
	// Después del trofeo (prioridad 0): una victoria en el último frame gana al fin por tiempo.
	set_physics_process_priority(100);
}

void MatchController::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint() || !is_running()) {
		return;
	}
	lv::core::PerfScope perf("match.tick");
	handle(flow_.advance(static_cast<float>(p_delta)));
}

void MatchController::start(float p_duration) {
	const lv::core::MatchFlow::Rules rules = rules_.is_valid() ? rules_->to_rules(p_duration) : [&] {
		lv::core::MatchFlow::Rules r;
		r.duration = p_duration;
		return r;
	}();
	flow_.start(rules);
	winner_ = nullptr;
	running_ = true;
	emit_signal("phase_changed", PHASE_PREPARATION, get_phase_name(PHASE_PREPARATION));
}

void MatchController::stop() {
	running_ = false;
}

void MatchController::declare_winner(Object *p_participant) {
	if (!is_running()) {
		return;
	}
	winner_ = p_participant;
	handle(flow_.declare_winner(0));
}

void MatchController::skip_phase() {
	if (is_running()) {
		handle(flow_.skip_phase());
	}
}

float MatchController::get_maze_speed() const {
	return rules_.is_valid() ? rules_->maze_speed(get_phase()) : (get_phase() == PHASE_HEIST ? 2.0f : 1.0f);
}

String MatchController::get_phase_name(int p_phase) {
	switch (p_phase) {
		case PHASE_PREPARATION:
			return String::utf8("Preparación");
		case PHASE_ASSAULT:
			return "Asalto";
		case PHASE_HEIST:
			return String::utf8("Golpe de los vándalos");
		default:
			return "Fin";
	}
}

void MatchController::handle(const std::vector<lv::core::MatchFlow::Event> &p_events) {
	using Type = lv::core::MatchFlow::Event::Type;
	for (const auto &event : p_events) {
		switch (event.type) {
			case Type::PhaseChanged:
				emit_signal("phase_changed", static_cast<int>(event.phase), get_phase_name(static_cast<int>(event.phase)));
				break;
			case Type::TimeWarning:
				emit_signal("time_warning", event.seconds_left);
				break;
			case Type::Ended:
				running_ = false;
				emit_signal("match_ended", static_cast<int>(event.outcome), event.outcome == lv::core::MatchFlow::Outcome::Victory ? winner_ : nullptr);
				break;
		}
	}
}
