#pragma once

#include "core/life_cycle.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/vector2.hpp>

using namespace godot;

// Ciclo de vida de un participante (eliminar → reaparecer en su base → protección), como componente hijo
// del Participant (composición, igual que StatsComponent). Envuelve a lv::core::LifeCycle (State).
//
// - eliminate(by): si es vulnerable, lo saca del juego `respawn_delay` s. Señal eliminated(by).
// - Al volver lo pone en `spawn_position` (su base) y queda protegido `protection_time` s. Señales
//   respawned y protection_ended.
// - No dibuja: la vista (participant_view.gd) escucha las señales para ocultarlo o hacerlo parpadear.
// Hoy lo usa el robo del trofeo ("atrapar al portador"); el combate (Etapa 4) agregará salud y vidas.
// Ver docs/ARQUITECTURA.md §11.
class LifeComponent : public Node {
	GDCLASS(LifeComponent, Node)

public:
	enum State {
		STATE_ALIVE,
		STATE_ELIMINATED,
		STATE_PROTECTED,
	};

	void _ready() override;
	void _physics_process(double p_delta) override;

	bool eliminate(Object *p_by);
	// Vuelve a empezar vivo (nueva partida).
	void reset();

	bool is_active() const { return life_.is_active(); }
	bool is_vulnerable() const { return life_.is_vulnerable(); }
	int get_state() const { return static_cast<int>(life_.state()); }
	float get_time_left() const { return life_.time_left(); }

	void set_respawn_delay(float p_value);
	float get_respawn_delay() const { return respawn_delay_; }
	void set_protection_time(float p_value);
	float get_protection_time() const { return protection_time_; }
	void set_spawn_position(const Vector2 &p_value) { spawn_position_ = p_value; }
	Vector2 get_spawn_position() const { return spawn_position_; }

protected:
	static void _bind_methods();

private:
	lv::core::LifeCycle life_;
	float respawn_delay_ = 3.0f;
	float protection_time_ = 2.0f;
	Vector2 spawn_position_;
};

VARIANT_ENUM_CAST(LifeComponent::State);
