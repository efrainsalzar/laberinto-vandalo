#include "game/health_component.h"

#include "core/perf.h"
#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>

void HealthComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("apply_damage", "amount", "by", "kind", "direction"), &HealthComponent::apply_damage, DEFVAL(Variant()), DEFVAL(DAMAGE_BULLET), DEFVAL(Vector2()));
	ClassDB::bind_method(D_METHOD("heal", "amount"), &HealthComponent::heal);
	ClassDB::bind_method(D_METHOD("reset"), &HealthComponent::reset);
	ClassDB::bind_method(D_METHOD("get_health"), &HealthComponent::get_health);
	ClassDB::bind_method(D_METHOD("get_ratio"), &HealthComponent::get_ratio);
	ClassDB::bind_method(D_METHOD("is_dead"), &HealthComponent::is_dead);
	ClassDB::bind_method(D_METHOD("set_max_health", "value"), &HealthComponent::set_max_health);
	ClassDB::bind_method(D_METHOD("get_max_health"), &HealthComponent::get_max_health);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_health", PROPERTY_HINT_RANGE, "1,1000,1"), "set_max_health", "get_max_health");

	BIND_ENUM_CONSTANT(DAMAGE_BULLET);
	BIND_ENUM_CONSTANT(DAMAGE_EXPLOSION);
	BIND_ENUM_CONSTANT(DAMAGE_BEAM);

	ADD_SIGNAL(MethodInfo("damaged", PropertyInfo(Variant::FLOAT, "amount"), PropertyInfo(Variant::OBJECT, "by"),
			PropertyInfo(Variant::INT, "kind"), PropertyInfo(Variant::VECTOR2, "direction")));
	ADD_SIGNAL(MethodInfo("healed", PropertyInfo(Variant::FLOAT, "amount")));
	ADD_SIGNAL(MethodInfo("health_changed", PropertyInfo(Variant::FLOAT, "current"), PropertyInfo(Variant::FLOAT, "max")));
	ADD_SIGNAL(MethodInfo("died", PropertyInfo(Variant::OBJECT, "by")));
}

void HealthComponent::_ready() {
	set_physics_process(false); // solo mientras haya láser pendiente de mostrar
}

void HealthComponent::set_max_health(float p_value) {
	health_.setup(p_value);
	emit_changed();
}

void HealthComponent::reset() {
	beam_batch_.flush();
	set_physics_process(false);
	health_.reset();
	emit_changed();
}

void HealthComponent::emit_changed() {
	emit_signal("health_changed", health_.current(), health_.max());
}

float HealthComponent::take_damage(const lv::core::DamageInfo &p_info, Participant *p_by) {
	const Participant *owner = Object::cast_to<Participant>(get_parent());
	if (owner != nullptr && !owner->is_vulnerable()) {
		return 0.0f; // eliminado o con protección
	}
	lv::core::PerfScope perf("combat.damage");
	const lv::core::DamageResult result = health_.damage(p_info);
	if (result.applied <= 0.0f) {
		return 0.0f;
	}
	const Vector2 direction(p_info.dir_x, p_info.dir_y);
	if (p_info.kind == lv::core::DamageKind::Beam) {
		beam_batch_.add(result.applied);
		beam_by_ = p_by != nullptr ? ObjectID(p_by->get_instance_id()) : ObjectID();
		beam_direction_ = direction;
		set_physics_process(true);
	} else {
		emit_signal("damaged", result.applied, p_by, static_cast<int>(p_info.kind), direction);
		emit_changed();
	}
	if (result.killed) {
		flush_beam(); // el número del láser sale antes de la muerte
		emit_signal("died", p_by);
	}
	return result.applied;
}

float HealthComponent::apply_damage(float p_amount, Object *p_by, int p_kind, const Vector2 &p_direction) {
	lv::core::DamageInfo info;
	info.amount = p_amount;
	info.kind = static_cast<lv::core::DamageKind>(p_kind);
	info.dir_x = p_direction.x;
	info.dir_y = p_direction.y;
	Participant *by = Object::cast_to<Participant>(p_by);
	info.source = by != nullptr ? by->get_participant_id() : -1;
	return take_damage(info, by);
}

float HealthComponent::heal(float p_amount) {
	const float healed = health_.heal(p_amount);
	if (healed > 0.0f) {
		emit_signal("healed", healed);
		emit_changed();
	}
	return healed;
}

void HealthComponent::flush_beam() {
	const float amount = beam_batch_.flush();
	set_physics_process(false);
	if (amount > 0.0f) {
		emit_signal("damaged", amount, ObjectDB::get_instance(beam_by_), static_cast<int>(DAMAGE_BEAM), beam_direction_);
		emit_changed();
	}
}

void HealthComponent::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	const float amount = beam_batch_.tick(static_cast<float>(p_delta));
	if (amount > 0.0f) {
		emit_signal("damaged", amount, ObjectDB::get_instance(beam_by_), static_cast<int>(DAMAGE_BEAM), beam_direction_);
		emit_changed();
	}
	if (!beam_batch_.has_pending()) {
		set_physics_process(false);
	}
}
