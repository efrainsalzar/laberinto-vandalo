#include "game/pickup_spawner.h"

#include "game/maze.h"
#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

void KeepDensityPolicy::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_target_count", "value"), &KeepDensityPolicy::set_target_count);
	ClassDB::bind_method(D_METHOD("get_target_count"), &KeepDensityPolicy::get_target_count);
	ClassDB::bind_method(D_METHOD("set_respawn_delay", "value"), &KeepDensityPolicy::set_respawn_delay);
	ClassDB::bind_method(D_METHOD("get_respawn_delay"), &KeepDensityPolicy::get_respawn_delay);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_count", PROPERTY_HINT_RANGE, "0,500,1"), "set_target_count", "get_target_count");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "respawn_delay", PROPERTY_HINT_RANGE, "0,60,0.1,suffix:s"), "set_respawn_delay", "get_respawn_delay");
}

void PickupSpawner::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start", "maze", "bases", "seed"), &PickupSpawner::start);
	ClassDB::bind_method(D_METHOD("stop"), &PickupSpawner::stop);
	ClassDB::bind_method(D_METHOD("set_factory_path", "value"), &PickupSpawner::set_factory_path);
	ClassDB::bind_method(D_METHOD("get_factory_path"), &PickupSpawner::get_factory_path);
	ClassDB::bind_method(D_METHOD("set_policy", "value"), &PickupSpawner::set_policy);
	ClassDB::bind_method(D_METHOD("get_policy"), &PickupSpawner::get_policy);
	ClassDB::bind_method(D_METHOD("set_observer_margin", "value"), &PickupSpawner::set_observer_margin);
	ClassDB::bind_method(D_METHOD("get_observer_margin"), &PickupSpawner::get_observer_margin);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "factory_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "PickupFactory"), "set_factory_path", "get_factory_path");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "policy", PROPERTY_HINT_RESOURCE_TYPE, "PickupSpawnPolicy"), "set_policy", "get_policy");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "observer_margin", PROPERTY_HINT_RANGE, "0,20,0.5,suffix:casillas"), "set_observer_margin", "get_observer_margin");

	ADD_SIGNAL(MethodInfo("pickup_spawned", PropertyInfo(Variant::OBJECT, "pickup", PROPERTY_HINT_NODE_TYPE, "Pickup")));
}

void PickupSpawner::start(Maze *p_maze, const TypedArray<Vector2i> &p_bases, int p_seed) {
	factory_ = Object::cast_to<PickupFactory>(get_node_or_null(factory_path_));
	if (factory_ == nullptr || p_maze == nullptr) {
		UtilityFunctions::push_error("PickupSpawner: falta el laberinto o `factory_path` no apunta a un PickupFactory.");
		maze_ = nullptr;
		return;
	}
	maze_ = p_maze;
	bases_.clear();
	for (int i = 0; i < p_bases.size(); i++) {
		const Vector2i base = p_bases[i];
		bases_.push_back({ base.x, base.y });
	}
	// Semilla derivada de la del laberinto: misma partida = mismas reapariciones.
	rng_ = std::make_unique<lv::core::Rng>(static_cast<uint32_t>(p_seed) ^ 0x9E3779B9u);
	cell_options_.min_spacing = factory_->get_min_spacing();
	cell_options_.base_clearance = factory_->get_base_clearance();
	factory_->populate(p_maze, p_bases, p_seed);
}

void PickupSpawner::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint() || maze_ == nullptr || policy_.is_null()) {
		return;
	}
	const int to_spawn = policy_->compute_spawns(factory_->get_active_count(), p_delta);
	for (int i = 0; i < to_spawn; i++) {
		if (Pickup *pickup = spawn_one()) {
			emit_signal("pickup_spawned", pickup);
		}
	}
}

Pickup *PickupSpawner::spawn_one() {
	const TypedArray<PickupType> types = factory_->get_types();
	std::vector<float> weights;
	for (int i = 0; i < types.size(); i++) {
		Ref<PickupType> type = types[i];
		weights.push_back(type.is_valid() ? type->get_weight() : 0.0f);
	}
	const int kind = rng_->weighted(weights);
	if (kind < 0) {
		return nullptr;
	}

	std::vector<lv::core::Cell> occupied;
	for (Pickup *pickup : factory_->get_active_pickups()) {
		const Vector2i cell = maze_->world_to_cell(pickup->get_global_position());
		occupied.push_back({ cell.x, cell.y });
	}
	std::vector<lv::core::SpawnObserver> observers;
	const TypedArray<Node> participants = get_tree()->get_nodes_in_group("participants");
	for (int i = 0; i < participants.size(); i++) {
		Participant *participant = Object::cast_to<Participant>(participants[i]);
		if (participant == nullptr) {
			continue;
		}
		lv::core::SpawnObserver observer;
		observer.position = maze_->world_to_grid_point(participant->get_global_position());
		observer.vision_radius = participant->get_stats() != nullptr ? participant->get_stats()->get_vision() : 5.0f;
		observers.push_back(observer);
	}

	const std::optional<lv::core::Cell> cell = lv::core::pick_spawn_cell(maze_->get_grid(), occupied, bases_, observers, cell_options_, *rng_);
	if (!cell) {
		return nullptr; // sin lugar ahora; la política lo volverá a pedir
	}
	const Vector2 world = maze_->cell_to_world(Vector2i(cell->x, cell->y));
	return factory_->create(types[kind], factory_->to_local(world));
}
