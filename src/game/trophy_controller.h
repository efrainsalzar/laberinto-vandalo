#pragma once

#include "core/trophy_tracker.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

class Maze;
class Participant;

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
//   Es la regla mínima para la primera prueba; el combate (Etapa 4) agregará otras formas (golpe, muerte).
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
	void set_steal_enabled(bool p_value) { steal_enabled_ = p_value; }
	bool is_steal_enabled() const { return steal_enabled_; }
	void set_steal_immunity(float p_value) { tracker_.set_steal_immunity(p_value); }
	float get_steal_immunity() const { return tracker_.steal_immunity(); }
	void set_steal_margin(float p_value) { steal_margin_ = p_value; }
	float get_steal_margin() const { return steal_margin_; }
	// Segundos que le quedan al portador sin que se lo puedan quitar (0 = se puede robar).
	float get_immunity_left() const { return tracker_.immunity_left(); }

	void set_carrier_speed_multiplier(float p_value) { carrier_speed_multiplier_ = p_value; }
	float get_carrier_speed_multiplier() const { return carrier_speed_multiplier_; }

protected:
	static void _bind_methods();

private:
	static inline const char *CARRIER_SOURCE = "trophy";

	lv::core::TrophyTracker tracker_;
	Maze *maze_ = nullptr;
	std::vector<Participant *> participants_;
	std::vector<Node2D *> base_views_;
	Node2D *trophy_view_ = nullptr;
	Ref<PackedScene> trophy_scene_;
	Ref<PackedScene> base_scene_;
	float carrier_speed_multiplier_ = 0.6f;
	bool steal_enabled_ = true;
	float steal_margin_ = 2.0f;

	void check_steal();

	void clear_views();
	void on_map_changed(const TypedArray<Vector2i> &p_tiles, bool p_now_wall);
	void handle(const std::vector<lv::core::TrophyTracker::Event> &p_events);
	void update_trophy_view();
	Participant *participant(int p_index) const;
	Vector2 cell_center(const lv::core::Cell &p_cell) const;
	Rect2 zone_world_rect(const lv::core::TileRect &p_zone) const;
};
