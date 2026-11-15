#pragma once

#include <vector>

namespace lv::core {

// Ciclo de vida de un participante (patrón State, como enum: pocos estados con tiempos).
//
//   Alive ──eliminate()──► Eliminated ──respawn_delay──► Protected ──protection──► Alive
//
// - Eliminated: no está en el juego (no se mueve, no toma nada); reaparece en su base al terminar la espera.
// - Protected: ya volvió, se mueve y juega, pero no se lo puede eliminar (evita "esperar en la base").
// - Hoy solo elimina atrapar al portador del trofeo; el combate (Etapa 4) agregará salud y vidas encima de
//   esta misma pieza (p. ej. con 0 vidas, quedarse en Eliminated para siempre).
// Ver docs/ARQUITECTURA.md §11.
class LifeCycle {
public:
	enum class State {
		Alive,
		Eliminated,
		Protected,
	};

	enum class Event {
		Eliminated,
		Respawned, // volvió (empieza la protección)
		ProtectionEnded,
	};

	void setup(float p_respawn_delay, float p_protection);
	// Elimina si es vulnerable (Alive). Devuelve true si lo eliminó.
	bool eliminate();
	// Avanza los tiempos; devuelve lo que pasó (en orden).
	std::vector<Event> tick(float p_delta);

	State state() const { return state_; }
	bool is_active() const { return state_ != State::Eliminated; } // está en el juego
	bool is_vulnerable() const { return state_ == State::Alive; }
	float time_left() const { return timer_; } // de la espera o de la protección
	float respawn_delay() const { return respawn_delay_; }
	float protection() const { return protection_; }

private:
	State state_ = State::Alive;
	float timer_ = 0.0f;
	float respawn_delay_ = 3.0f;
	float protection_ = 2.0f;
};

} // namespace lv::core
