#pragma once

#include "core/bot_brain.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/rect2i.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <deque>
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
// Ver docs/ARQUITECTURA.md §19.
class BotCoordinator : public Node {
	GDCLASS(BotCoordinator, Node)

public:
	static constexpr int MAX_BOTS = 7; // + 1 humano = 8 participantes (VISION §5)
	static constexpr int MAX_SEEN_PICKUPS = 8;

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
	std::deque<PathRequest> requests_;
	int max_paths_per_frame_ = 2;

	void on_map_changed(const TypedArray<Vector2i> &p_tiles, bool p_now_wall);
};
