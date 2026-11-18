#pragma once

#include "core/participant_command.h"
#include "game/health_component.h"
#include "game/life_component.h"
#include "game/participant_controller.h"
#include "game/stats_component.h"
#include "game/weapon_component.h"

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
// - Si tiene un WeaponComponent hijo, la orden de disparo (fire + aim) va a esa arma (§22).
// - Su salud está en un HealthComponent hijo (§25); al reaparecer vuelve a la salud máxima.
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
	// Los componentes se buscan al pedirlos (y se recuerdan): los hijos hacen su _ready ANTES que el padre, así que
	// si la vista o el arma los piden en su _ready, el participante todavía no los habría buscado.
	StatsComponent *get_stats() const { return find_component(stats_); }
	// Ciclo de vida (eliminar / reaparecer). Puede ser nullptr: entonces siempre está activo.
	LifeComponent *get_life() const { return find_component(life_); }
	bool is_active() const { return get_life() == nullptr || get_life()->is_active(); }
	bool is_vulnerable() const { return get_life() == nullptr || get_life()->is_vulnerable(); }
	// Salud (primer hijo HealthComponent, §25). Puede ser nullptr: entonces un golpe elimina directo.
	HealthComponent *get_health() const { return find_component(health_); }
	// Arma (primer hijo WeaponComponent). Puede ser nullptr: entonces no dispara.
	WeaponComponent *get_weapon() const { return find_component(weapon_); }

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
	mutable StatsComponent *stats_ = nullptr;
	mutable LifeComponent *life_ = nullptr;
	mutable HealthComponent *health_ = nullptr;
	mutable WeaponComponent *weapon_ = nullptr;

	// Primer hijo del tipo T (se recuerda en r_cache). nullptr si no hay.
	template <typename T>
	T *find_component(T *&r_cache) const {
		if (r_cache == nullptr) {
			for (int i = 0; i < get_child_count() && r_cache == nullptr; i++) {
				r_cache = Object::cast_to<T>(get_child(i));
			}
		}
		return r_cache;
	}
	uint32_t active_layer_ = 2; // capas de física mientras está en juego (se apagan al ser eliminado)
	uint32_t active_mask_ = 1;

	void on_eliminated(Object *p_by);
	void on_respawned();
	Vector2 facing_ = Vector2(0, 1);
	float body_radius_ = 11.0f;

	void apply_body_radius();
};
