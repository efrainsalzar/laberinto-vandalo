#include "game/shield_component.h"

#include "core/perf.h"
#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <memory>

void ShieldComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_shield", "kind", "duration"), &ShieldComponent::add_shield, DEFVAL(0.0f));
	ClassDB::bind_method(D_METHOD("clear"), &ShieldComponent::clear);
	ClassDB::bind_method(D_METHOD("has_shield", "kind"), &ShieldComponent::has_shield);
	ClassDB::bind_method(D_METHOD("get_time_left", "kind"), &ShieldComponent::get_time_left);
	ClassDB::bind_method(D_METHOD("get_shields"), &ShieldComponent::get_shields);
	ClassDB::bind_method(D_METHOD("is_empty"), &ShieldComponent::is_empty);
	ClassDB::bind_method(D_METHOD("set_bubble_duration", "value"), &ShieldComponent::set_bubble_duration);
	ClassDB::bind_method(D_METHOD("get_bubble_duration"), &ShieldComponent::get_bubble_duration);
	ClassDB::bind_method(D_METHOD("set_frontal_duration", "value"), &ShieldComponent::set_frontal_duration);
	ClassDB::bind_method(D_METHOD("get_frontal_duration"), &ShieldComponent::get_frontal_duration);
	ClassDB::bind_method(D_METHOD("set_frontal_half_angle", "value"), &ShieldComponent::set_frontal_half_angle);
	ClassDB::bind_method(D_METHOD("get_frontal_half_angle"), &ShieldComponent::get_frontal_half_angle);
	ClassDB::bind_method(D_METHOD("set_mirror_duration", "value"), &ShieldComponent::set_mirror_duration);
	ClassDB::bind_method(D_METHOD("get_mirror_duration"), &ShieldComponent::get_mirror_duration);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bubble_duration", PROPERTY_HINT_RANGE, "1,120,0.5,suffix:s"), "set_bubble_duration", "get_bubble_duration");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frontal_duration", PROPERTY_HINT_RANGE, "1,60,0.5,suffix:s"), "set_frontal_duration", "get_frontal_duration");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frontal_half_angle", PROPERTY_HINT_RANGE, "10,180,1,suffix:grados"), "set_frontal_half_angle", "get_frontal_half_angle");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mirror_duration", PROPERTY_HINT_RANGE, "1,60,0.5,suffix:s"), "set_mirror_duration", "get_mirror_duration");

	BIND_ENUM_CONSTANT(SHIELD_BUBBLE);
	BIND_ENUM_CONSTANT(SHIELD_FRONTAL);
	BIND_ENUM_CONSTANT(SHIELD_MIRROR);

	ADD_SIGNAL(MethodInfo("shield_added", PropertyInfo(Variant::INT, "kind")));
	ADD_SIGNAL(MethodInfo("shield_ended", PropertyInfo(Variant::INT, "kind")));
	ADD_SIGNAL(MethodInfo("blocked", PropertyInfo(Variant::INT, "kind"), PropertyInfo(Variant::BOOL, "reflected")));
}

void ShieldComponent::_ready() {
	set_physics_process(false); // solo mientras haya escudos
}

void ShieldComponent::add_shield(int p_kind, float p_duration) {
	std::unique_ptr<lv::core::Shield> shield;
	switch (static_cast<lv::core::ShieldKind>(p_kind)) {
		case lv::core::ShieldKind::Bubble:
			shield = std::make_unique<lv::core::BubbleShield>(p_duration > 0.0f ? p_duration : bubble_duration_);
			break;
		case lv::core::ShieldKind::Frontal:
			shield = std::make_unique<lv::core::FrontalShield>(p_duration > 0.0f ? p_duration : frontal_duration_, frontal_half_angle_);
			break;
		case lv::core::ShieldKind::Mirror:
			shield = std::make_unique<lv::core::MirrorShield>(p_duration > 0.0f ? p_duration : mirror_duration_);
			break;
		default:
			return;
	}
	chain_.add(std::move(shield));
	set_physics_process(!Engine::get_singleton()->is_editor_hint());
	emit_signal("shield_added", p_kind);
}

lv::core::ShieldResult ShieldComponent::filter(const lv::core::DamageInfo &p_info) {
	if (chain_.empty()) {
		return {};
	}
	lv::core::PerfScope perf("combat.shields");
	lv::core::ShieldContext context;
	if (const Participant *owner = Object::cast_to<Participant>(get_parent())) {
		const Vector2 aim = owner->get_aim();
		context.aim_x = aim.x;
		context.aim_y = aim.y;
	}
	const lv::core::ShieldResult result = chain_.filter(p_info, context);
	if (result.outcome == lv::core::ShieldOutcome::Pass) {
		return result;
	}
	const bool beam = p_info.kind == lv::core::DamageKind::Beam;
	if (!beam || beam_feedback_cooldown_ <= 0.0f) {
		if (beam) {
			beam_feedback_cooldown_ = BEAM_FEEDBACK_SECONDS;
		}
		emit_signal("blocked", static_cast<int>(result.by), result.outcome == lv::core::ShieldOutcome::Reflected);
	}
	emit_ended(); // la burbuja se rompe en el acto
	return result;
}

void ShieldComponent::emit_ended() {
	ended_.clear();
	chain_.remove_spent(ended_);
	for (const lv::core::ShieldKind kind : ended_) {
		emit_signal("shield_ended", static_cast<int>(kind));
	}
}

void ShieldComponent::clear() {
	ended_.clear();
	for (const auto &link : chain_.links()) {
		ended_.push_back(link->kind());
	}
	chain_.clear();
	set_physics_process(false);
	for (const lv::core::ShieldKind kind : ended_) {
		emit_signal("shield_ended", static_cast<int>(kind));
	}
}

void ShieldComponent::_physics_process(double p_delta) {
	const float dt = static_cast<float>(p_delta);
	beam_feedback_cooldown_ -= dt;
	ended_.clear();
	chain_.tick(dt, ended_);
	for (const lv::core::ShieldKind kind : ended_) {
		emit_signal("shield_ended", static_cast<int>(kind));
	}
	if (chain_.empty()) {
		set_physics_process(false);
	}
}

bool ShieldComponent::has_shield(int p_kind) const {
	return chain_.has(static_cast<lv::core::ShieldKind>(p_kind));
}

float ShieldComponent::get_time_left(int p_kind) const {
	const lv::core::Shield *shield = chain_.find(static_cast<lv::core::ShieldKind>(p_kind));
	return shield != nullptr ? shield->time_left() : 0.0f;
}

Array ShieldComponent::get_shields() const {
	Array result;
	for (const auto &link : chain_.links()) {
		Dictionary d;
		d["kind"] = static_cast<int>(link->kind());
		d["remaining"] = link->time_left();
		d["duration"] = link->duration();
		result.push_back(d);
	}
	return result;
}
