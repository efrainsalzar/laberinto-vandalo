#include "game/participant_controller.h"

#include "game/participant.h"

#include <godot_cpp/classes/input.hpp>

lv::core::ParticipantCommand ParticipantController::compute_command(const Participant &, double) {
	return {};
}

lv::core::ParticipantCommand HumanController::compute_command(const Participant &p_participant, double) {
	// Acciones definidas en project.godot ([input]): WASD y flechas; fire y aim_* (mouse / mando).
	Input *input = Input::get_singleton();
	const Vector2 direction = input->get_vector("move_left", "move_right", "move_up", "move_down");
	lv::core::ParticipantCommand command;
	command.move_x = direction.x;
	command.move_y = direction.y;

	command.fire = input->is_action_pressed("fire");
	command.use_item = input->is_action_pressed("use_item");
	// Stick derecho si se está usando; si no, hacia el puntero del mouse.
	Vector2 aim = input->get_vector("aim_left", "aim_right", "aim_up", "aim_down");
	if (aim.is_zero_approx()) {
		aim = p_participant.get_global_mouse_position() - p_participant.get_global_position();
	}
	command.aim_x = aim.x;
	command.aim_y = aim.y;
	return command;
}

lv::core::ParticipantCommand WanderController::compute_command(const Participant &p_participant, double p_delta) {
	if (!rng_) {
		rng_ = std::make_unique<lv::core::Rng>(static_cast<uint32_t>(get_instance_id()));
		pick_direction();
	}
	timer_ -= static_cast<float>(p_delta);
	const bool stuck = p_participant.get_real_velocity().length_squared() < 4.0f;
	if (timer_ <= 0.0f || stuck) {
		pick_direction();
	}
	lv::core::ParticipantCommand command;
	command.move_x = dir_x_;
	command.move_y = dir_y_;
	return command;
}

void WanderController::pick_direction() {
	static const float DIRS[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
	const int d = rng_->range(4);
	dir_x_ = DIRS[d][0];
	dir_y_ = DIRS[d][1];
	timer_ = 0.4f + rng_->unit() * 1.2f;
}