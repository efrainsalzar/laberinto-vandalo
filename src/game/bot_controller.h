#pragma once

#include "core/bot_brain.h"
#include "core/bot_gunner.h"
#include "core/random.h"
#include "game/participant_controller.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/rect2i.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <memory>
#include <vector>

using namespace godot;

class BotCoordinator;
class Maze;

// Dificultad de un bot como datos (.tres): las "perillas" de lv::core::BotTuning.
// Una dificultad o personalidad nueva = un .tres nuevo, sin código. Ejemplos: project/data/bots/*.tres
class BotProfile : public Resource {
	GDCLASS(BotProfile, Resource)

public:
	void set_display_name(const String &p_value) { display_name_ = p_value; }
	String get_display_name() const { return display_name_; }
	void set_reaction_time(float p_value) { tuning_.reaction_time = p_value; }
	float get_reaction_time() const { return tuning_.reaction_time; }
	void set_think_interval(float p_value) { tuning_.think_interval = p_value; }
	float get_think_interval() const { return tuning_.think_interval; }
	void set_mistake_chance(float p_value) { tuning_.mistake_chance = p_value; }
	float get_mistake_chance() const { return tuning_.mistake_chance; }
	void set_pickup_interest(float p_value) { tuning_.pickup_interest = p_value; }
	float get_pickup_interest() const { return tuning_.pickup_interest; }
	void set_variation(float p_value) { tuning_.variation = p_value; }
	float get_variation() const { return tuning_.variation; }
	// Puntería: cada disparo se desvía al azar hasta ± este ángulo (§22).
	void set_aim_error_deg(float p_value) { aim_error_deg_ = p_value; }
	float get_aim_error_deg() const { return aim_error_deg_; }

	const lv::core::BotTuning &tuning() const { return tuning_; }

protected:
	static void _bind_methods();

private:
	String display_name_ = "Normal";
	lv::core::BotTuning tuning_;
	float aim_error_deg_ = 12.0f;
};

// Controlador de un bot (Strategy): el Participant no sabe que lo controla una IA.
//
// - Decide con lv::core::BotBrain (Utility AI): qué hacer y a qué casilla ir.
// - Navega con un camino de celdas lógicas (A*, lv::core::find_path) que calcula BotCoordinator con
//   presupuesto por frame; lo sigue de centro a centro de celda y produce un ParticipantCommand (Command).
// - En los cruces puede equivocarse (mistake_chance) y luego replantea el camino.
// - Si queda trabado o el mapa cierra su camino, pide uno nuevo (BotCoordinator le avisa).
// - Dispara mientras se mueve: lv::core::BotGunner elige al rival visible más cercano y apunta con el error
//   de su perfil; va en el mismo ParticipantCommand (fire + aim). Ver §22.
// Lo crea BotCoordinator.add_bot (no se instancia a mano). Ver docs/ARQUITECTURA.md §19.
class BotController : public ParticipantController {
	GDCLASS(BotController, ParticipantController)

public:
	lv::core::ParticipantCommand compute_command(const Participant &p_participant, double p_delta) override;
	// Volvió a su base tras ser eliminado: descarta el camino viejo y piensa de nuevo.
	void on_respawned(const Participant &p_participant) override;

	// --- Usado por BotCoordinator ---
	void setup(BotCoordinator *p_coordinator, const Ref<BotProfile> &p_profile, const Rect2i &p_base_zone, uint32_t p_seed);
	void detach() { coordinator_ = nullptr; }
	// Camino calculado (celdas) hacia p_target_world. Vacío = no hay camino. p_generation: la del pedido
	// (si el bot reapareció entre medio, el camino es de antes y se descarta).
	void apply_path(const std::vector<int> &p_cells, const Vector2 &p_target_world, int p_generation);
	int get_generation() const { return generation_; }
	// ¿El mapa cambió y su camino ya no sirve? Entonces pide otro.
	void validate_path();
	void clear_path_request() { path_requested_ = false; }

	Ref<BotProfile> get_profile() const { return profile_; }
	// Objetivo actual (BotGoal como entero; útil para depurar o mostrar en pantalla).
	int get_goal() const;
	bool has_path() const { return !path_.empty(); }

protected:
	static void _bind_methods();

private:
	static constexpr float STUCK_SECONDS = 1.0f;
	static constexpr float AIM_SCAN_INTERVAL = 0.15f; // cada cuánto revisa a qué rivales ve

	lv::core::ParticipantCommand compute_move(const Participant &p_participant, float p_delta);
	void aim(const Participant &p_participant, float p_delta, lv::core::ParticipantCommand &r_command);

	BotCoordinator *coordinator_ = nullptr;
	Ref<BotProfile> profile_;
	std::unique_ptr<lv::core::BotBrain> brain_;
	std::unique_ptr<lv::core::Rng> rng_;
	lv::core::TileRect base_;
	lv::core::BotWorld world_;
	bool world_ready_ = false;
	lv::core::Cell explore_target_;
	bool has_explore_target_ = false;

	std::vector<int> path_;
	size_t path_index_ = 0;
	int previous_cell_ = -1;
	Vector2 target_world_;
	bool path_requested_ = false;
	int generation_ = 0; // sube al reaparecer
	bool replan_at_end_ = false; // se desvió por un error: al terminar el desvío, replantea

	lv::core::Cell requested_target_; // destino del pedido en curso (si el objetivo cambia, se descarta)
	Vector2 last_position_;
	float last_radius_ = 11.0f;

	lv::core::BotGunner gunner_;
	std::vector<ObjectID> seen_rivals_; // rivales visibles en la última revisión (ids: pueden desaparecer)
	std::vector<lv::core::GunTarget> gun_targets_; // reutilizado cada frame
	float aim_scan_timer_ = 0.0f;

	float retry_timer_ = 0.0f; // espera antes de volver a pedir camino si no hubo ruta
	float stuck_timer_ = 0.0f;
	float best_distance_ = 0.0f;

	void request_path(const Participant &p_participant);
	void pick_explore_target();
	Vector2 waypoint(size_t p_index) const;
	void advance_waypoint(const Participant &p_participant);
};
