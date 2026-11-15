#pragma once

#include "core/bot_brain.h"
#include "core/token_pool.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/rect2i.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <deque>
#include <unordered_map>
#include <vector>

using namespace godot;

class BotController;
class BotProfile;
class Maze;
class Participant;
class PickupFactory;
class TrophyController;

// Coordina a todos los bots de la partida.
//
// - Factory: add_bot() crea el BotController de un participante con su perfil, base y semilla.
// - Cola de caminos con presupuesto: los bots PIDEN un camino y aquí se calculan como máximo
//   `max_paths_per_frame` por frame (A* sobre celdas). Un pedido repetido del mismo bot no se duplica.
// - Observer: escucha `Maze.map_changed` y avisa a cada bot para que revise si su camino sigue abierto.
// - Percepción: arma lo que cada bot sabe (BotWorld): trofeo (visible para todos), su base y los objetos
//   que VE (línea de visión, con el radio de su stat de visión).
// - Fichas de ataque (TokenPool): cuando alguien lleva el trofeo, solo los `max_chasers` bots más cercanos
//   (TrophyRules) pueden perseguirlo; los demás emboscan en el camino a la base del portador. Se reparten
//   cada TOKEN_INTERVAL s, no en cada frame.
// Ver docs/ARQUITECTURA.md §19.
class BotCoordinator : public Node {
	GDCLASS(BotCoordinator, Node)

public:
	static constexpr int MAX_BOTS = 7; // + 1 humano = 8 participantes (VISION §5)
	static constexpr int MAX_SEEN_PICKUPS = 8;
	static constexpr float TOKEN_INTERVAL = 0.25f;
	static constexpr int AMBUSH_CELLS_FROM_BASE = 2; // a cuántas celdas de la base espera la emboscada

	~BotCoordinator() override { clear(); } // los bots no deben quedar apuntando a un coordinador borrado
	void _physics_process(double p_delta) override;

	void setup(Maze *p_maze, TrophyController *p_trophy, PickupFactory *p_pickups);
	// Crea y asigna el controlador del bot. Devuelve null si se superó MAX_BOTS.
	Ref<BotController> add_bot(Participant *p_participant, const Ref<BotProfile> &p_profile, const Rect2i &p_base_zone, int p_seed);
	void clear();
	int get_bot_count() const { return static_cast<int>(bots_.size()); }

	void set_max_paths_per_frame(int p_value) { max_paths_per_frame_ = p_value < 1 ? 1 : p_value; }
	int get_max_paths_per_frame() const { return max_paths_per_frame_; }

	// --- Usado por BotController ---
	Maze *get_maze() const { return maze_; }
	void request_path(const Ref<BotController> &p_bot, const Vector2i &p_from_tile, const Vector2i &p_to_tile);
	void build_world(const Participant &p_participant, lv::core::BotWorld &r_world) const;

protected:
	static void _bind_methods();

private:
	struct PathRequest {
		Ref<BotController> bot;
		Vector2i from;
		Vector2i to;
	};

	Maze *maze_ = nullptr;
	TrophyController *trophy_ = nullptr;
	PickupFactory *pickups_ = nullptr;
	std::vector<Ref<BotController>> bots_;
	std::vector<Participant *> bot_bodies_; // bot_bodies_[i] es el participante de bots_[i]
	lv::core::TokenPool chase_tokens_;
	float token_timer_ = 0.0f;
	// Punto de emboscada por portador (se calcula una vez; se borra cuando el mapa cambia).
	mutable std::unordered_map<const Participant *, lv::core::Cell> ambush_cache_;
	std::deque<PathRequest> requests_;
	int max_paths_per_frame_ = 2;

	void on_map_changed(const TypedArray<Vector2i> &p_tiles, bool p_now_wall);
	void update_chase_tokens();
	bool ambush_for(const Participant *p_carrier, lv::core::Cell &r_cell) const;
	int bot_index(const Participant *p_participant) const;
};
