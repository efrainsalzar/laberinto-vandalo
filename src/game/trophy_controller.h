#pragma once

#include "core/trophy_tracker.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

class Maze;
class Participant;

// Reglas del trofeo y del robo como datos (.tres): se ajustan sin recompilar.
// Ejemplo: project/data/trophy/default_rules.tres. Ver docs/ARQUITECTURA.md §11.
class TrophyRules : public Resource {
	GDCLASS(TrophyRules, Resource)

public:
	// Qué pasa al tocar al portador (Strategy por datos; el combate agregará "por golpe").
	enum StealMode {
		STEAL_TRANSFER = 0, // el trofeo pasa a quien toca (con impulso y aturdimiento, abajo)
		STEAL_ELIMINATE = 1, // el portador queda eliminado, reaparece en su base y el trofeo cae al piso
	};

	float carrier_speed_multiplier = 0.85f; // lentitud del portador
	bool steal_enabled = true;
	int steal_mode = STEAL_ELIMINATE;
	float steal_immunity = 2.0f; // s sin que se lo quiten tras tomarlo o robarlo
	float steal_margin = 2.0f; // px extra de alcance al tocar
	float drop_lock = 0.5f; // s en que nadie puede tomar el trofeo recién caído
	// Solo modo Transferir (anti-bucle): el ladrón escapa y la víctima queda aturdida.
	float thief_boost_multiplier = 1.5f;
	float thief_boost_seconds = 2.0f;
	float victim_stun_multiplier = 0.2f;
	float victim_stun_seconds = 1.0f;
	// IA: cuántos bots pueden perseguir al portador a la vez (fichas de ataque); el resto embosca.
	int max_chasers = 2;

#define LV_RULE(type, name)                           \
	void set_##name(type p_value) { name = p_value; } \
	type get_##name() const { return name; }
	LV_RULE(float, carrier_speed_multiplier)
	LV_RULE(bool, steal_enabled)
	LV_RULE(int, steal_mode)
	LV_RULE(float, drop_lock)
	LV_RULE(float, steal_immunity)
	LV_RULE(float, steal_margin)
	LV_RULE(float, thief_boost_multiplier)
	LV_RULE(float, thief_boost_seconds)
	LV_RULE(float, victim_stun_multiplier)
	LV_RULE(float, victim_stun_seconds)
	LV_RULE(int, max_chasers)
#undef LV_RULE

protected:
	static void _bind_methods();
};

VARIANT_ENUM_CAST(TrophyRules::StealMode);

// Conecta las reglas del trofeo (lv::core::TrophyTracker) con el mundo de Godot.
//
// - Cada frame de física pasa la casilla de cada participante al tracker y traduce sus eventos
//   a señales (Observer): trophy_taken, trophy_dropped, trophy_delivered.
// - Al portador le aplica un modificador de velocidad (×carrier_speed_multiplier, origen "trophy")
//   y se lo quita al soltarlo o entregarlo (reutiliza Stats, §6).
// - Crea la vista del trofeo y de cada base a partir de escenas (`trophy_scene`, `base_scene`):
//   no dibuja nada. A las vistas les pasa datos con set(): `color`, `size_px`, `carried`.
// - Robo básico (`steal_enabled`): quien TOCA al portador (los cuerpos se superponen + `steal_margin` px) le
//   quita el trofeo; el nuevo portador queda inmune `steal_immunity` s. Señal trophy_stolen(thief, victim).
//   Modo según `rules.steal_mode`: ELIMINAR (por defecto: el portador sale del juego unos segundos, reaparece
//   en su base y el trofeo cae al piso) o TRANSFERIR (el trofeo pasa al que toca, con impulso y aturdimiento).
//   Todos los valores vienen de `rules` (TrophyRules, datos). El combate (Etapa 4) agregará otras formas de robo.
// - No sabe dónde está la bóveda en el mundo: recibe la casilla del trofeo (DISENO_MAPA §3).
// Ver docs/ARQUITECTURA.md §11.
class TrophyController : public Node2D {
	GDCLASS(TrophyController, Node2D)

public:
	void _physics_process(double p_delta) override;

	// Prepara una partida: participants[i] tendrá la base i de maze.get_base_cells(n).
	void start(Maze *p_maze, const TypedArray<Participant> &p_participants);
	// El portador suelta el trofeo donde está (lo usará la muerte en la fase de combate).
	void drop_trophy();

	Participant *get_carrier() const;
	Participant *get_winner() const;
	bool is_finished() const { return tracker_.state() == lv::core::TrophyTracker::State::Delivered; }
	// Dónde está el trofeo en el mundo (sobre el portador si alguien lo lleva).
	Vector2 get_trophy_position() const;
	// Centro de la zona de resguardo del participante (en el mundo).
	Vector2 get_base_position(Participant *p_participant) const;

	void set_trophy_scene(const Ref<PackedScene> &p_value) { trophy_scene_ = p_value; }
	Ref<PackedScene> get_trophy_scene() const { return trophy_scene_; }
	void set_base_scene(const Ref<PackedScene> &p_value) { base_scene_ = p_value; }
	Ref<PackedScene> get_base_scene() const { return base_scene_; }
	void set_rules(const Ref<TrophyRules> &p_value);
	Ref<TrophyRules> get_rules() const { return rules_; }
	// Reglas vigentes (las de `rules`, o las por defecto si no hay).
	const TrophyRules &rules() const;
	float get_steal_immunity() const { return rules().steal_immunity; }
	// Segundos que le quedan al portador sin que se lo puedan quitar (0 = se puede robar).
	float get_immunity_left() const { return tracker_.immunity_left(); }

protected:
	static void _bind_methods();

private:
	static inline const char *CARRIER_SOURCE = "trophy";
	static inline const char *BOOST_SOURCE = "steal_boost";
	static inline const char *STUN_SOURCE = "steal_stun";

	lv::core::TrophyTracker tracker_;
	Maze *maze_ = nullptr;
	std::vector<Participant *> participants_;
	std::vector<Node2D *> base_views_;
	Node2D *trophy_view_ = nullptr;
	Ref<PackedScene> trophy_scene_;
	Ref<PackedScene> base_scene_;
	Ref<TrophyRules> rules_;
	Ref<TrophyRules> default_rules_;

	void check_steal();
	void eliminate_carrier(Participant *p_carrier, Participant *p_by);

	void clear_views();
	void on_map_changed(const TypedArray<Vector2i> &p_tiles, bool p_now_wall);
	void handle(const std::vector<lv::core::TrophyTracker::Event> &p_events);
	void update_trophy_view();
	Participant *participant(int p_index) const;
	Vector2 cell_center(const lv::core::Cell &p_cell) const;
	Rect2 zone_world_rect(const lv::core::TileRect &p_zone) const;
};
