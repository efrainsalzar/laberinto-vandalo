#include "game/pickup_spawner.h"

#include "core/perf.h"

#include "game/maze.h"
#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>

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
	ClassDB::bind_method(D_METHOD("set_categories", "value"), &PickupSpawner::set_categories);
	ClassDB::bind_method(D_METHOD("get_categories"), &PickupSpawner::get_categories);
	ClassDB::bind_method(D_METHOD("set_budget", "value"), &PickupSpawner::set_budget);
	ClassDB::bind_method(D_METHOD("get_budget"), &PickupSpawner::get_budget);
	ClassDB::bind_method(D_METHOD("set_observer_margin", "value"), &PickupSpawner::set_observer_margin);
	ClassDB::bind_method(D_METHOD("get_observer_margin"), &PickupSpawner::get_observer_margin);
	ClassDB::bind_method(D_METHOD("get_category_target", "index"), &PickupSpawner::get_category_target);
	ClassDB::bind_method(D_METHOD("get_category_active", "index"), &PickupSpawner::get_category_active);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "factory_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "PickupFactory"), "set_factory_path", "get_factory_path");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "categories", PROPERTY_HINT_TYPE_STRING,
						 UtilityFunctions::str(Variant::OBJECT) + "/" + UtilityFunctions::str(PROPERTY_HINT_RESOURCE_TYPE) + ":PickupCategory"),
			"set_categories", "get_categories");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "budget", PROPERTY_HINT_RANGE, "0,2000,1"), "set_budget", "get_budget");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "observer_margin", PROPERTY_HINT_RANGE, "0,20,0.5,suffix:casillas"), "set_observer_margin", "get_observer_margin");

	ADD_SIGNAL(MethodInfo("pickup_spawned", PropertyInfo(Variant::OBJECT, "pickup", PROPERTY_HINT_NODE_TYPE, "Pickup")));
	ADD_SIGNAL(MethodInfo("pickup_expired", PropertyInfo(Variant::OBJECT, "pickup", PROPERTY_HINT_NODE_TYPE, "Pickup")));
}

void PickupSpawner::start(Maze *p_maze, const TypedArray<Vector2i> &p_bases, int p_seed) {
	factory_ = Object::cast_to<PickupFactory>(get_node_or_null(factory_path_));
	if (factory_ == nullptr || p_maze == nullptr) {
		UtilityFunctions::push_error("PickupSpawner: falta el laberinto o `factory_path` no apunta a un PickupFactory.");
		maze_ = nullptr;
		return;
	}
	if (maze_ != nullptr && maze_->is_connected("map_changed", callable_mp(this, &PickupSpawner::on_map_changed))) {
		maze_->disconnect("map_changed", callable_mp(this, &PickupSpawner::on_map_changed));
	}
	maze_ = p_maze;
	// Si una pared se cierra sobre un objeto, se retira; la política lo repone en otro lugar.
	maze_->connect("map_changed", callable_mp(this, &PickupSpawner::on_map_changed));
	bases_.clear();
	for (int i = 0; i < p_bases.size(); i++) {
		const Vector2i base = p_bases[i];
		bases_.push_back({ base.x, base.y });
	}
	// Semilla derivada de la del laberinto: misma partida = mismas reapariciones.
	rng_ = std::make_unique<lv::core::Rng>(static_cast<uint32_t>(p_seed) ^ 0x9E3779B9u);
	cell_options_.min_spacing = factory_->get_min_spacing();
	cell_options_.base_clearance = factory_->get_base_clearance();

	// Partida nueva: borrar lo anterior ANTES de reiniciar los contadores (borrar avisa a on_pickup_exiting).
	generation_++;
	factory_->begin(p_maze);
	expiry_.clear();
	now_ = 0.0;
	states_.clear();

	for (int i = 0; i < categories_.size(); i++) {
		CategoryState state;
		state.category = categories_[i];
		if (state.category.is_valid()) {
			const Ref<PickupCategory> &c = state.category;
			state.target = lv::core::category_target(budget_, c->get_share(), c->get_min_count(), c->get_max_count());
			if (c->get_policy().is_valid()) {
				state.policy = c->get_policy()->duplicate();
				state.policy->configure(state.target, c->get_refill_time());
			}
		}
		states_.push_back(state);
	}

	// Objetos iniciales. Su vida se reparte (40-100 %) para que no desaparezcan todos a la vez.
	for (int i = 0; i < static_cast<int>(states_.size()); i++) {
		const Ref<PickupCategory> c = states_[i].category;
		if (c.is_null()) {
			continue;
		}
		const int initial = static_cast<int>(std::lround(states_[i].target * c->get_initial_fill()));
		const int category_seed = p_seed + 7919 * (i + 1);
		for (Pickup *pickup : factory_->place(p_bases, c->get_types(), initial, category_seed)) {
			track(pickup, i, c->get_lifetime() * (0.4 + 0.6 * rng_->unit()));
		}
	}
}

int PickupSpawner::get_category_target(int p_index) const {
	return p_index >= 0 && p_index < static_cast<int>(states_.size()) ? states_[p_index].target : 0;
}

int PickupSpawner::get_category_active(int p_index) const {
	return p_index >= 0 && p_index < static_cast<int>(states_.size()) ? states_[p_index].active : 0;
}

void PickupSpawner::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint() || maze_ == nullptr) {
		return;
	}
	lv::core::PerfScope perf("pickups.spawner");
	now_ += p_delta;
	process_expiry();
	for (int i = 0; i < static_cast<int>(states_.size()); i++) {
		if (states_[i].policy.is_null()) {
			continue;
		}
		const int to_spawn = states_[i].policy->compute_spawns(states_[i].active, p_delta);
		for (int n = 0; n < to_spawn; n++) {
			if (Pickup *pickup = spawn_one(i)) {
				track(pickup, i, states_[i].category->get_lifetime());
				emit_signal("pickup_spawned", pickup);
			}
		}
	}
}

void PickupSpawner::track(Pickup *p_pickup, int p_category, double p_lifetime) {
	states_[p_category].active++;
	// Cualquier salida (recogido, caducado, pared encima, partida nueva) descuenta del cupo.
	p_pickup->connect("tree_exiting", callable_mp(this, &PickupSpawner::on_pickup_exiting).bind(p_category, generation_), CONNECT_ONE_SHOT);
	const Ref<PickupCategory> c = states_[p_category].category;
	expiry_.schedule(p_pickup->get_instance_id(), now_, p_lifetime, c.is_valid() ? c->get_warn_time() : 0.0);
}

void PickupSpawner::on_pickup_exiting(int p_category, int p_generation) {
	if (p_generation != generation_ || p_category < 0 || p_category >= static_cast<int>(states_.size())) {
		return;
	}
	states_[p_category].active = std::max(0, states_[p_category].active - 1);
}

void PickupSpawner::process_expiry() {
	for (const lv::core::ExpiryQueue::Event &event : expiry_.poll(now_)) {
		Pickup *pickup = Object::cast_to<Pickup>(ObjectDB::get_instance(ObjectID(event.id)));
		if (pickup == nullptr || pickup->is_consumed() || !pickup->is_inside_tree()) {
			continue; // ya lo recogieron o lo retiró una pared
		}
		if (event.stage == lv::core::ExpiryQueue::Stage::Warn) {
			pickup->warn_expiring();
		} else {
			emit_signal("pickup_expired", pickup);
			factory_->remove_pickup(pickup);
		}
	}
}

Pickup *PickupSpawner::spawn_one(int p_category) {
	const TypedArray<PickupType> types = states_[p_category].category->get_types();
	std::vector<float> weights;
	for (int i = 0; i < types.size(); i++) {
		Ref<PickupType> type = types[i];
		weights.push_back(type.is_valid() ? type->get_weight() : 0.0f);
	}
	const int kind = rng_->weighted(weights);
	if (kind < 0) {
		return nullptr;
	}

	PickupFactory *factory = factory_;
	const lv::core::HasPickupAt has_pickup = [factory](const lv::core::Cell &p_cell) {
		return factory->has_pickup_at(Vector2i(p_cell.x, p_cell.y));
	};
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

	const std::optional<lv::core::Cell> cell = lv::core::pick_spawn_cell_indexed(maze_->get_grid(), has_pickup, bases_, observers, cell_options_, *rng_);
	if (!cell) {
		return nullptr; // sin lugar ahora; la política lo volverá a pedir
	}
	const Vector2 world = maze_->cell_to_world(Vector2i(cell->x, cell->y));
	return factory_->create(types[kind], factory_->to_local(world));
}

void PickupSpawner::on_map_changed(const TypedArray<Vector2i> &p_tiles, bool p_now_wall) {
	if (p_now_wall && factory_ != nullptr) {
		factory_->remove_pickups_at(maze_, p_tiles);
	}
}
