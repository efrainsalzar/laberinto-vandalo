#pragma once

#include "core/match_flow.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Reglas de tiempo de una partida como datos (.tres): parte de cada fase, avisos y lo que cambia en cada fase.
// La DURACIÓN no está aquí: la elige el jugador en el menú (MatchConfig) y se pasa a MatchController.start().
// Ejemplo: project/data/match/default_rules.tres
class MatchRules : public Resource {
	GDCLASS(MatchRules, Resource)

public:
	void set_preparation_share(float p_value) { preparation_share_ = p_value; }
	float get_preparation_share() const { return preparation_share_; }
	void set_assault_share(float p_value) { assault_share_ = p_value; }
	float get_assault_share() const { return assault_share_; }
	void set_heist_share(float p_value) { heist_share_ = p_value; }
	float get_heist_share() const { return heist_share_; }
	void set_warnings(const PackedFloat32Array &p_value) { warnings_ = p_value; }
	PackedFloat32Array get_warnings() const { return warnings_; }
	// Ritmo del laberinto vivo en cada fase (1 = normal, 2 = el doble de cambios).
	void set_preparation_maze_speed(float p_value) { maze_speed_[0] = p_value; }
	float get_preparation_maze_speed() const { return maze_speed_[0]; }
	void set_assault_maze_speed(float p_value) { maze_speed_[1] = p_value; }
	float get_assault_maze_speed() const { return maze_speed_[1]; }
	void set_heist_maze_speed(float p_value) { maze_speed_[2] = p_value; }
	float get_heist_maze_speed() const { return maze_speed_[2]; }

	float maze_speed(int p_phase) const { return (p_phase >= 0 && p_phase < 3) ? maze_speed_[p_phase] : 1.0f; }
	lv::core::MatchFlow::Rules to_rules(float p_duration) const;

protected:
	static void _bind_methods();

private:
	float preparation_share_ = 0.40f;
	float assault_share_ = 0.35f;
	float heist_share_ = 0.25f;
	PackedFloat32Array warnings_ = PackedFloat32Array({ 60.0f, 10.0f });
	float maze_speed_[3] = { 1.0f, 1.0f, 2.0f };
};

// La partida: reloj y fases (Preparación → Asalto → Golpe de los vándalos → fin).
//
// Envuelve a lv::core::MatchFlow (patrón State) y traduce sus eventos a señales (Observer):
//   phase_changed(phase, name) · time_warning(seconds_left) · match_ended(outcome, winner)
// La escena las reenvía al Event Bus. No sabe nada del laberinto, el trofeo ni la interfaz: quien gana se
// lo dicen con declare_winner(); qué cambia en cada fase lo decide quien escucha (con los datos de `rules`).
// Procesa en _physics_process DESPUÉS del trofeo (process_physics_priority alto): si alguien entrega el
// trofeo en el mismo frame en que se acaba el tiempo, gana la victoria.
// La pausa de Godot detiene el reloj (nodo pausable).
// Ver docs/ARQUITECTURA.md §18.
class MatchController : public Node {
	GDCLASS(MatchController, Node)

public:
	enum Phase {
		PHASE_PREPARATION,
		PHASE_ASSAULT,
		PHASE_HEIST,
		PHASE_ENDED,
	};
	enum Outcome {
		OUTCOME_NONE,
		OUTCOME_VICTORY,
		OUTCOME_TIME_UP,
	};

	MatchController();
	void _physics_process(double p_delta) override;

	// Empieza una partida. p_duration en segundos; <= 0 = sin límite.
	void start(float p_duration);
	void stop();
	// Alguien llevó el trofeo a su base.
	void declare_winner(Object *p_participant);
	// Teclas de prueba: salta a la fase siguiente (en el Golpe con límite: fin por tiempo).
	void skip_phase();

	int get_phase() const { return static_cast<int>(flow_.phase()); }
	int get_outcome() const { return static_cast<int>(flow_.outcome()); }
	Object *get_winner() const { return winner_; }
	bool is_running() const { return running_ && flow_.is_running(); }
	bool is_unlimited() const { return flow_.is_unlimited(); }
	float get_elapsed() const { return flow_.elapsed(); }
	float get_time_left() const { return flow_.time_left(); }
	float get_phase_time_left() const { return flow_.phase_time_left(); }
	// Ritmo del laberinto vivo en la fase actual (de `rules`).
	float get_maze_speed() const;
	static String get_phase_name(int p_phase);

	void set_rules(const Ref<MatchRules> &p_value) { rules_ = p_value; }
	Ref<MatchRules> get_rules() const { return rules_; }

protected:
	static void _bind_methods();

private:
	lv::core::MatchFlow flow_;
	Ref<MatchRules> rules_;
	Object *winner_ = nullptr;
	bool running_ = false;

	void handle(const std::vector<lv::core::MatchFlow::Event> &p_events);
};

VARIANT_ENUM_CAST(MatchController::Phase);
VARIANT_ENUM_CAST(MatchController::Outcome);
