#include "game/life_component.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/core/class_db.hpp>

void LifeComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("eliminate", "by"), &LifeComponent::eliminate);
	ClassDB::bind_method(D_METHOD("reset"), &LifeComponent::reset);
	ClassDB::bind_method(D_METHOD("is_active"), &LifeComponent::is_active);
	ClassDB::bind_method(D_METHOD("is_vulnerable"), &LifeComponent::is_vulnerable);
	ClassDB::bind_method(D_METHOD("get_state"), &LifeComponent::get_state);
	ClassDB::bind_method(D_METHOD("get_time_left"), &LifeComponent::get_time_left);
	ClassDB::bind_method(D_METHOD("set_respawn_delay", "value"), &LifeComponent::set_respawn_delay);
	ClassDB::bind_method(D_METHOD("get_respawn_delay"), &LifeComponent::get_respawn_delay);
	ClassDB::bind_method(D_METHOD("set_protection_time", "value"), &LifeComponent::set_protection_time);
	ClassDB::bind_method(D_METHOD("get_protection_time"), &LifeComponent::get_protection_time);
	ClassDB::bind_method(D_METHOD("set_spawn_position", "value"), &LifeComponent::set_spawn_position);
	ClassDB::bind_method(D_METHOD("get_spawn_position"), &LifeComponent::get_spawn_position);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "respawn_delay", PROPERTY_HINT_RANGE, "0,30,0.1,suffix:s"), "set_respawn_delay", "get_respawn_delay");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "protection_time", PROPERTY_HINT_RANGE, "0,10,0.1,suffix:s"), "set_protection_time", "get_protection_time");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "spawn_position"), "set_spawn_position", "get_spawn_position");

	BIND_ENUM_CONSTANT(STATE_ALIVE);
	BIND_ENUM_CONSTANT(STATE_ELIMINATED);
	BIND_ENUM_CONSTANT(STATE_PROTECTED);

	ADD_SIGNAL(MethodInfo("eliminated", PropertyInfo(Variant::OBJECT, "by")));
	ADD_SIGNAL(MethodInfo("respawned"));
	ADD_SIGNAL(MethodInfo("protection_ended"));
}

void LifeComponent::_ready() {
	life_.setup(respawn_delay_, protection_time_);
}

void LifeComponent::reset() {
	life_.setup(respawn_delay_, protection_time_);
}

void LifeComponent::set_respawn_delay(float p_value) {
	respawn_delay_ = p_value;
	if (life_.state() == lv::core::LifeCycle::State::Alive) {
		life_.setup(respawn_delay_, protection_time_);
	}
}

void LifeComponent::set_protection_time(float p_value) {
	protection_time_ = p_value;
	if (life_.state() == lv::core::LifeCycle::State::Alive) {
		life_.setup(respawn_delay_, protection_time_);
	}
}

bool LifeComponent::eliminate(Object *p_by) {
	if (!life_.eliminate()) {
		return false;
	}
	emit_signal("eliminated", p_by);
	return true;
}

void LifeComponent::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint() || life_.state() == lv::core::LifeCycle::State::Alive) {
		return;
	}
	for (const lv::core::LifeCycle::Event event : life_.tick(static_cast<float>(p_delta))) {
		switch (event) {
			case lv::core::LifeCycle::Event::Respawned:
				if (Node2D *body = Object::cast_to<Node2D>(get_parent())) {
					body->set_global_position(spawn_position_); // vuelve a su base
				}
				emit_signal("respawned");
				break;
			case lv::core::LifeCycle::Event::ProtectionEnded:
				emit_signal("protection_ended");
				break;
			case lv::core::LifeCycle::Event::Eliminated:
				break;
		}
	}
}
