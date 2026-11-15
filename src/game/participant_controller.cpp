#include "game/participant_controller.h"

#include <godot_cpp/classes/input.hpp>

lv::core::ParticipantCommand ParticipantController::compute_command(const Participant &, double) {
	return {};
}

lv::core::ParticipantCommand HumanController::compute_command(const Participant &, double) {
	// Acciones definidas en project.godot ([input]): WASD y flechas.
	const Vector2 direction = Input::get_singleton()->get_vector("move_left", "move_right", "move_up", "move_down");
	lv::core::ParticipantCommand command;
	command.move_x = direction.x;
	command.move_y = direction.y;
	return command;
}
