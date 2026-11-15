#include "game/pickup.h"

#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

void Pickup::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_type", "type"), &Pickup::set_type);
	ClassDB::bind_method(D_METHOD("get_type"), &Pickup::get_type);
	ClassDB::bind_method(D_METHOD("collect", "participant"), &Pickup::collect);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "type", PROPERTY_HINT_RESOURCE_TYPE, "PickupType"), "set_type", "get_type");

	ADD_SIGNAL(MethodInfo("collected", PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_NODE_TYPE, "Participant")));
}

void Pickup::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	// Solo detecta cuerpos; nadie necesita detectarlo a él.
	set_collision_layer(0);
	connect("body_entered", callable_mp(this, &Pickup::on_body_entered));
}

void Pickup::on_body_entered(Node2D *p_body) {
	if (Participant *participant = Object::cast_to<Participant>(p_body)) {
		collect(participant);
	}
}

bool Pickup::collect(Participant *p_participant) {
	if (consumed_ || p_participant == nullptr) {
		return false;
	}
	consumed_ = true;

	StatsComponent *stats = p_participant->get_stats();
	if (stats != nullptr && type_.is_valid()) {
		const String source = "pickup:" + type_->get_id();
		const TypedArray<PickupEffect> effects = type_->get_effects();
		for (int i = 0; i < effects.size(); i++) {
			Ref<PickupEffect> effect = effects[i];
			if (effect.is_valid()) {
				stats->add_modifier(effect->get_stat(), effect->get_op(), effect->get_value(), effect->get_duration(), source);
			}
		}
	}

	emit_signal("collected", p_participant);
	set_deferred("monitoring", false);
	queue_free();
	return true;
}
