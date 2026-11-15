#include "game/participant.h"

#include <godot_cpp/classes/circle_shape2d.hpp>
#include <godot_cpp/classes/collision_shape2d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

void Participant::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_controller", "controller"), &Participant::set_controller);
	ClassDB::bind_method(D_METHOD("get_controller"), &Participant::get_controller);
	ClassDB::bind_method(D_METHOD("set_participant_id", "value"), &Participant::set_participant_id);
	ClassDB::bind_method(D_METHOD("get_participant_id"), &Participant::get_participant_id);
	ClassDB::bind_method(D_METHOD("set_display_name", "value"), &Participant::set_display_name);
	ClassDB::bind_method(D_METHOD("get_display_name"), &Participant::get_display_name);
	ClassDB::bind_method(D_METHOD("set_color", "value"), &Participant::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &Participant::get_color);
	ClassDB::bind_method(D_METHOD("get_stats"), &Participant::get_stats);
	ClassDB::bind_method(D_METHOD("get_life"), &Participant::get_life);
	ClassDB::bind_method(D_METHOD("is_active"), &Participant::is_active);
	ClassDB::bind_method(D_METHOD("is_vulnerable"), &Participant::is_vulnerable);
	ClassDB::bind_method(D_METHOD("get_facing"), &Participant::get_facing);
	ClassDB::bind_method(D_METHOD("set_body_radius", "value"), &Participant::set_body_radius);
	ClassDB::bind_method(D_METHOD("get_body_radius"), &Participant::get_body_radius);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "participant_id"), "set_participant_id", "get_participant_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "body_radius", PROPERTY_HINT_RANGE, "2,32,0.5,suffix:px"), "set_body_radius", "get_body_radius");

	ADD_SIGNAL(MethodInfo("color_changed", PropertyInfo(Variant::COLOR, "color")));
	ADD_SIGNAL(MethodInfo("body_radius_changed", PropertyInfo(Variant::FLOAT, "radius")));
}

void Participant::_ready() {
	add_to_group("participants"); // lo usan sistemas que necesitan a todos (p. ej. PickupSpawner)
	apply_body_radius();
	for (int i = 0; i < get_child_count(); i++) {
		if (stats_ == nullptr) {
			stats_ = Object::cast_to<StatsComponent>(get_child(i));
		}
		if (life_ == nullptr) {
			life_ = Object::cast_to<LifeComponent>(get_child(i));
		}
	}
	if (life_ != nullptr && !Engine::get_singleton()->is_editor_hint()) {
		life_->connect("eliminated", callable_mp(this, &Participant::on_eliminated));
		life_->connect("respawned", callable_mp(this, &Participant::on_respawned));
	}
	if (stats_ == nullptr && !Engine::get_singleton()->is_editor_hint()) {
		UtilityFunctions::push_warning("Participant '", display_name_, "' no tiene StatsComponent: no se moverá.");
	}
}

void Participant::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint() || controller_.is_null()) {
		return;
	}
	if (!is_active()) {
		set_velocity(Vector2()); // eliminado: fuera del juego hasta reaparecer
		return;
	}
	execute_command(controller_->compute_command(*this, p_delta));
}

void Participant::execute_command(const lv::core::ParticipantCommand &p_command) {
	Vector2 direction(p_command.move_x, p_command.move_y);
	direction = direction.limit_length(1.0f); // en diagonal no se va más rápido
	if (!direction.is_zero_approx()) {
		facing_ = direction.normalized();
	}
	const float speed = stats_ != nullptr ? stats_->get_speed() : 0.0f;
	set_velocity(direction * speed);
	move_and_slide();
}

void Participant::set_controller(const Ref<ParticipantController> &p_controller) {
	controller_ = p_controller;
}

void Participant::set_color(const Color &p_value) {
	color_ = p_value;
	emit_signal("color_changed", color_);
}

void Participant::set_body_radius(float p_value) {
	body_radius_ = p_value < 2.0f ? 2.0f : p_value;
	if (is_inside_tree()) {
		apply_body_radius();
	}
}

void Participant::apply_body_radius() {
	for (int i = 0; i < get_child_count(); i++) {
		CollisionShape2D *collision = Object::cast_to<CollisionShape2D>(get_child(i));
		if (collision == nullptr) {
			continue;
		}
		Ref<CircleShape2D> circle = collision->get_shape();
		if (circle.is_null()) {
			continue;
		}
		if (circle->get_radius() == body_radius_) {
			break;
		}
		// La forma viene de la escena y se comparte entre instancias: cada participante usa su copia.
		if (!circle->is_local_to_scene()) {
			circle = circle->duplicate();
			circle->set_local_to_scene(true);
			collision->set_shape(circle);
		}
		circle->set_radius(body_radius_);
		break;
	}
	emit_signal("body_radius_changed", body_radius_);
}

// Eliminado: sin colisión (no toma objetos ni toca a nadie) hasta reaparecer.
void Participant::on_eliminated(Object *) {
	active_layer_ = get_collision_layer();
	active_mask_ = get_collision_mask();
	set_collision_layer(0);
	set_collision_mask(0);
}

void Participant::on_respawned() {
	set_collision_layer(active_layer_);
	set_collision_mask(active_mask_);
	if (controller_.is_valid()) {
		controller_->on_respawned(*this);
	}
}
