#pragma once

#include "core/participant_command.h"
#include "game/participant_controller.h"

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Un participante del torneo: el cuerpo que se mueve por el laberinto.
//
// - No sabe quién lo controla: tiene un ParticipantController (Strategy) que le da
//   un ParticipantCommand (Command) en cada frame, y lo ejecuta.
// - No se dibuja a sí mismo: la parte visual vive en su escena (participant.tscn).
// - Las stats (velocidad, visión...) serán un componente propio en la pieza 3;
//   por ahora la velocidad es una propiedad simple.
// Ver docs/ARQUITECTURA.md §5.
class Participant : public CharacterBody2D {
	GDCLASS(Participant, CharacterBody2D)

public:
	void _physics_process(double p_delta) override;

	// Ejecuta una orden. Público para que red/repeticiones puedan inyectar órdenes.
	void execute_command(const lv::core::ParticipantCommand &p_command);

	void set_controller(const Ref<ParticipantController> &p_controller);
	Ref<ParticipantController> get_controller() const { return controller_; }

	void set_participant_id(int p_value) { participant_id_ = p_value; }
	int get_participant_id() const { return participant_id_; }
	void set_display_name(const String &p_value) { display_name_ = p_value; }
	String get_display_name() const { return display_name_; }
	void set_color(const Color &p_value);
	Color get_color() const { return color_; }
	void set_move_speed(float p_value) { move_speed_ = p_value; }
	float get_move_speed() const { return move_speed_; }

	// Última dirección en la que se movió (para orientar sprites o la visión).
	Vector2 get_facing() const { return facing_; }

protected:
	static void _bind_methods();

private:
	Ref<ParticipantController> controller_;
	int participant_id_ = 0;
	String display_name_ = "Participante";
	Color color_ = Color(0.35f, 0.75f, 1.0f);
	float move_speed_ = 160.0f; // píxeles por segundo
	Vector2 facing_ = Vector2(0, 1);
};
