#pragma once

#include <vector>

namespace lv::core {

// Reloj y fases de una partida (VISION §4, §16). Lógica pura: avanza con el tiempo y devuelve lo que pasó.
//
// Fases (patrón State, como enum: hoy las fases no tienen comportamiento propio; quien quiera reaccionar
// escucha los eventos):
//   Preparation ──share──► Assault ──share──► Heist ──fin del reloj──► Ended (TimeUp)
//        └───────────── declare_winner() en cualquier fase ──────────────► Ended (Victory)
//
// - advance(dt) devuelve TODOS los eventos ocurridos en ese tramo y en orden: si un frame largo cruza
//   dos fases, salen los dos cambios de fase (nada se pierde).
// - Duración <= 0 = sin límite: las fases avanzan con una duración de referencia, el Golpe dura para
//   siempre y no hay fin por tiempo ni avisos.
// - Empate en el mismo instante: si se declara ganador antes de procesar el fin del reloj, gana la
//   victoria (el nodo de Godot procesa el reloj después del trofeo).
// Ver docs/ARQUITECTURA.md §18.
class MatchFlow {
public:
	enum class Phase {
		Preparation,
		Assault,
		Heist,
		Ended,
	};

	enum class Outcome {
		None,
		Victory, // alguien llevó el trofeo a su base
		TimeUp, // se acabó el tiempo: los vándalos se llevan el trofeo, nadie gana
	};

	struct Rules {
		float duration = 480.0f; // segundos; <= 0 = sin límite
		// Parte del tiempo de cada fase. Se normalizan (no hace falta que sumen 1).
		float preparation_share = 0.40f;
		float assault_share = 0.35f;
		float heist_share = 0.25f;
		// Avisos de "quedan X segundos" (cada uno se emite una vez). Se ignoran los >= duración.
		std::vector<float> warnings = { 60.0f, 10.0f };
		// Duración usada para repartir las fases cuando no hay límite.
		float unlimited_reference = 480.0f;
	};

	struct Event {
		enum class Type {
			PhaseChanged, // empezó `phase`
			TimeWarning, // quedan `seconds_left` segundos
			Ended, // terminó con `outcome` (y `winner` si hubo victoria)
		};
		Type type;
		Phase phase = Phase::Preparation;
		float seconds_left = 0.0f;
		Outcome outcome = Outcome::None;
		int winner = -1;
	};

	void start(const Rules &p_rules);
	std::vector<Event> advance(float p_delta);
	// Alguien ganó (trofeo entregado). Sin efecto si la partida ya terminó.
	std::vector<Event> declare_winner(int p_participant);
	// Salta al comienzo de la fase siguiente (teclas de prueba). Desde el Golpe con límite = fin por tiempo.
	std::vector<Event> skip_phase();

	Phase phase() const { return phase_; }
	Outcome outcome() const { return outcome_; }
	int winner() const { return winner_; }
	bool is_running() const { return started_ && phase_ != Phase::Ended; }
	bool is_unlimited() const { return rules_.duration <= 0.0f; }
	float elapsed() const { return elapsed_; }
	// Segundos que quedan de partida (-1 si no hay límite).
	float time_left() const;
	// Segundos que quedan de la fase actual (-1 si no termina por tiempo).
	float phase_time_left() const;
	// Momento (segundos desde el inicio) en que empieza cada fase. Ended = fin del reloj.
	float phase_start(Phase p_phase) const;

private:
	Rules rules_;
	bool started_ = false;
	Phase phase_ = Phase::Preparation;
	Outcome outcome_ = Outcome::None;
	int winner_ = -1;
	float elapsed_ = 0.0f;
	float bounds_[3] = { 0.0f, 0.0f, 0.0f }; // fin de Preparation, Assault y Heist
	std::vector<bool> warned_;

	void emit_until(float p_time, std::vector<Event> &r_events);
};

} // namespace lv::core
