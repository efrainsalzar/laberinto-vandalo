#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

void Participant::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_controller", "controller"), &Participant::set_controller);
	ClassDB::bind_method(D_METHOD("get_controller"), &Participant::get_controller);
	ClassDB::bind_method(D_METHOD("set_participant_id", "value"), &Participant::set_participant_id);
	ClassDB::bind_method(D_METHOD("get_participant_id"), &Participant::get_participant_id);
	ClassDB::bind_method(D_METHOD("set_display_name", "value"), &Participant::set_display_name);
	ClassDB::bind_method(D_METHOD("get_display_name"), &Participant::get_display_name);
	ClassDB::bind_method(D_METHOD("set_color", "value"), &Participant::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &Participant::get_color);
	ClassDB::bind_method(D_METHOD("set_move_speed", "value"), &Participant::set_move_speed);
	ClassDB::bind_method(D_METHOD("get_move_speed"), &Participant::get_move_speed);
	ClassDB::bind_method(D_METHOD("get_facing"), &Participant::get_facing);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "participant_id"), "set_participant_id", "get_participant_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "move_speed", PROPERTY_HINT_RANGE, "10,600,1,suffix:px/s"), "set_move_speed", "get_move_speed");

	ADD_SIGNAL(MethodInfo("color_changed", PropertyInfo(Variant::COLOR, "color")));
}

void Participant::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint() || controller_.is_null()) {
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
	set_velocity(direction * move_speed_);
	move_and_slide();
}

void Participant::set_controller(const Ref<ParticipantController> &p_controller) {
	controller_ = p_controller;
}

void Participant::set_color(const Color &p_value) {
	color_ = p_value;
	emit_signal("color_changed", color_);
}
