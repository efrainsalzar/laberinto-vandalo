#pragma once

#include "core/random.h"
#include "game/pickup_factory.h"
#include "game/pickup_spawn_policy.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <memory>

using namespace godot;

class Maze;

// Hace que los objetos sean "infinitos": coloca los iniciales y repone los que se recogen.
//
// - CUÁNTOS y CUÁNDO: lo decide su `policy` (Strategy: PickupSpawnPolicy).
// - DÓNDE: lv::core::pick_spawn_cell, nunca a la vista de un participante (grupo "participants").
// - CÓMO se crean: lo hace el PickupFactory (Factory).
// Emite `pickup_spawned` (Observer) para efectos visuales o el Director de IA.
// Ver docs/ARQUITECTURA.md §9.
class PickupSpawner : public Node {
	GDCLASS(PickupSpawner, Node)

public:
	void _process(double p_delta) override;

	// Coloca los objetos iniciales y empieza a reponer. Llamar tras generar el laberinto.
	void start(Maze *p_maze, const TypedArray<Vector2i> &p_bases, int p_seed);
	void stop() { maze_ = nullptr; }

	void set_factory_path(const NodePath &p_value) { factory_path_ = p_value; }
	NodePath get_factory_path() const { return factory_path_; }
	void set_policy(const Ref<PickupSpawnPolicy> &p_value) { policy_ = p_value; }
	Ref<PickupSpawnPolicy> get_policy() const { return policy_; }
	void set_observer_margin(float p_value) { cell_options_.observer_margin = p_value; }
	float get_observer_margin() const { return cell_options_.observer_margin; }

protected:
	static void _bind_methods();

private:
	NodePath factory_path_;
	Ref<PickupSpawnPolicy> policy_;
	lv::core::SpawnCellOptions cell_options_;
	Maze *maze_ = nullptr;
	PickupFactory *factory_ = nullptr;
	std::vector<lv::core::Cell> bases_;
	std::unique_ptr<lv::core::Rng> rng_;

	Pickup *spawn_one();
	void on_map_changed(const TypedArray<Vector2i> &p_tiles, bool p_now_wall);
};
