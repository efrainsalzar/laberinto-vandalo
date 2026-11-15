#pragma once

#include "core/participant_command.h"
#include "game/participant_controller.h"
#include "game/stats_component.h"

#include <godot_cpp/classes/character_body2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Un participante del torneo: el cuerpo que se mueve por el laberinto.
//
// - No sabe quién lo controla: tiene un ParticipantController (Strategy) que le da
//   un ParticipantCommand (Command) en cada frame, y lo ejecuta.
// - No se dibuja a sí mismo: la parte visual vive en su escena (participant.tscn).
// - Sus stats (velocidad, visión...) están en un StatsComponent hijo (composición).
// Ver docs/ARQUITECTURA.md §5.
class Participant : public CharacterBody2D {
	GDCLASS(Participant, CharacterBody2D)

public:
	void _ready() override;
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
	// Componente de stats (primer hijo de tipo StatsComponent). Puede ser nullptr.
	StatsComponent *get_stats() const { return stats_; }

	// Radio del cuerpo (px): ajusta el círculo de colisión y avisa a la vista (señal body_radius_changed).
	// La partida lo adapta al ancho de pasillo para que el participante siempre quepa.
	void set_body_radius(float p_value);
	float get_body_radius() const { return body_radius_; }

	// Última dirección en la que se movió (para orientar sprites o la visión).
	Vector2 get_facing() const { return facing_; }

protected:
	static void _bind_methods();

private:
	Ref<ParticipantController> controller_;
	int participant_id_ = 0;
	String display_name_ = "Participante";
	Color color_ = Color(0.35f, 0.75f, 1.0f);
	StatsComponent *stats_ = nullptr;
	Vector2 facing_ = Vector2(0, 1);
	float body_radius_ = 11.0f;

	void apply_body_radius();
};
