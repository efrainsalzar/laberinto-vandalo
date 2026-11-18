#include "game/pickup_factory.h"

#include "core/perf.h"
#include "core/pickup_placer.h"
#include "game/maze.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <vector>

void PickupFactory::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create", "type", "position"), &PickupFactory::create);
	ClassDB::bind_method(D_METHOD("populate", "maze", "bases", "seed"), &PickupFactory::populate);
	ClassDB::bind_method(D_METHOD("clear_pickups"), &PickupFactory::clear_pickups);
	ClassDB::bind_method(D_METHOD("remove_pickups_at", "maze", "tiles"), &PickupFactory::remove_pickups_at);
	ClassDB::bind_method(D_METHOD("get_active_count"), &PickupFactory::get_active_count);
	ClassDB::bind_method(D_METHOD("set_pickup_scene", "value"), &PickupFactory::set_pickup_scene);
	ClassDB::bind_method(D_METHOD("get_pickup_scene"), &PickupFactory::get_pickup_scene);
	ClassDB::bind_method(D_METHOD("set_types", "value"), &PickupFactory::set_types);
	ClassDB::bind_method(D_METHOD("get_types"), &PickupFactory::get_types);
	ClassDB::bind_method(D_METHOD("set_count", "value"), &PickupFactory::set_count);
	ClassDB::bind_method(D_METHOD("get_count"), &PickupFactory::get_count);
	ClassDB::bind_method(D_METHOD("set_min_spacing", "value"), &PickupFactory::set_min_spacing);
	ClassDB::bind_method(D_METHOD("get_min_spacing"), &PickupFactory::get_min_spacing);
	ClassDB::bind_method(D_METHOD("set_base_clearance", "value"), &PickupFactory::set_base_clearance);
	ClassDB::bind_method(D_METHOD("get_base_clearance"), &PickupFactory::get_base_clearance);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "pickup_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_pickup_scene", "get_pickup_scene");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "types", PROPERTY_HINT_TYPE_STRING,
						 UtilityFunctions::str(Variant::OBJECT) + "/" + UtilityFunctions::str(PROPERTY_HINT_RESOURCE_TYPE) + ":PickupType"),
			"set_types", "get_types");
	ADD_GROUP("Colocación", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "count", PROPERTY_HINT_RANGE, "0,500,1"), "set_count", "get_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "min_spacing", PROPERTY_HINT_RANGE, "1,20,1,suffix:casillas"), "set_min_spacing", "get_min_spacing");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "base_clearance", PROPERTY_HINT_RANGE, "0,30,1,suffix:casillas"), "set_base_clearance", "get_base_clearance");

	ADD_SIGNAL(MethodInfo("pickup_collected",
			PropertyInfo(Variant::OBJECT, "pickup", PROPERTY_HINT_NODE_TYPE, "Pickup"),
			PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_NODE_TYPE, "Participant")));
}

Pickup *PickupFactory::create(const Ref<PickupType> &p_type, const Vector2 &p_position) {
	if (pickup_scene_.is_null()) {
		UtilityFunctions::push_error("PickupFactory: falta `pickup_scene`.");
		return nullptr;
	}
	Pickup *pickup = Object::cast_to<Pickup>(pickup_scene_->instantiate());
	if (pickup == nullptr) {
		UtilityFunctions::push_error("PickupFactory: la raíz de `pickup_scene` debe ser un Pickup.");
		return nullptr;
	}
	pickup->set_type(p_type);
	pickup->set_position(p_position);
	pickup->connect("collected", callable_mp(this, &PickupFactory::on_pickup_collected).bind(pickup));
	add_child(pickup);
	index_add(pickup);
	active_count_++;
	return pickup;
}

int PickupFactory::populate(Maze *p_maze, const TypedArray<Vector2i> &p_bases, int p_seed) {
	begin(p_maze);
	return static_cast<int>(place(p_bases, types_, count_, p_seed).size());
}

void PickupFactory::begin(Maze *p_maze) {
	clear_pickups();
	maze_ = p_maze;
}

std::vector<Pickup *> PickupFactory::place(const TypedArray<Vector2i> &p_bases, const TypedArray<PickupType> &p_types, int p_count, int p_seed) {
	lv::core::PerfScope perf("pickups.populate");
	std::vector<Pickup *> created;
	if (maze_ == nullptr || p_types.is_empty() || p_count <= 0) {
		return created;
	}

	std::vector<float> weights;
	for (int i = 0; i < p_types.size(); i++) {
		Ref<PickupType> type = p_types[i];
		weights.push_back(type.is_valid() ? type->get_weight() : 0.0f);
	}
	std::vector<lv::core::Cell> bases;
	for (int i = 0; i < p_bases.size(); i++) {
		const Vector2i base = p_bases[i];
		bases.push_back({ base.x, base.y });
	}
	lv::core::PickupPlacementOptions options;
	options.count = p_count;
	options.min_spacing = min_spacing_;
	options.base_clearance = base_clearance_;
	options.seed = static_cast<uint32_t>(p_seed);

	for (const lv::core::PickupPlacement &placement : lv::core::place_pickups(maze_->get_grid(), weights, bases, options)) {
		const Vector2i cell(placement.cell.x, placement.cell.y);
		if (has_pickup_at(cell)) {
			continue; // ya hay un objeto de otra categoría
		}
		if (Pickup *pickup = create(p_types[placement.kind], to_local(maze_->cell_to_world(cell)))) {
			created.push_back(pickup);
		}
	}
	return created;
}

void PickupFactory::remove_pickup(Pickup *p_pickup) {
	if (p_pickup == nullptr || p_pickup->is_consumed() || p_pickup->get_parent() != this) {
		return;
	}
	index_remove(p_pickup);
	active_count_--;
	remove_child(p_pickup);
	p_pickup->queue_free();
}

void PickupFactory::index_add(Pickup *p_pickup) {
	if (maze_ != nullptr) {
		by_cell_[cell_key(maze_->world_to_cell(p_pickup->get_global_position()))] = p_pickup;
	}
}

void PickupFactory::index_remove(Pickup *p_pickup) {
	if (maze_ == nullptr) {
		return;
	}
	const auto it = by_cell_.find(cell_key(maze_->world_to_cell(p_pickup->get_global_position())));
	if (it != by_cell_.end() && it->second == p_pickup) {
		by_cell_.erase(it);
	}
}

bool PickupFactory::has_pickup_at(const Vector2i &p_cell) const {
	const auto it = by_cell_.find(cell_key(p_cell));
	return it != by_cell_.end() && it->second != nullptr && !it->second->is_consumed();
}

void PickupFactory::clear_pickups() {
	by_cell_.clear();
	active_count_ = 0;
	for (int i = get_child_count() - 1; i >= 0; i--) {
		if (Pickup *pickup = Object::cast_to<Pickup>(get_child(i))) {
			remove_child(pickup);
			pickup->queue_free();
		}
	}
}

int PickupFactory::remove_pickups_at(Maze *p_maze, const TypedArray<Vector2i> &p_tiles) {
	lv::core::PerfScope perf("pickups.remove");
	if (p_maze == nullptr) {
		return 0;
	}
	// O(casillas cambiadas) gracias al índice espacial.
	int removed = 0;
	for (int i = 0; i < p_tiles.size(); i++) {
		const auto it = by_cell_.find(cell_key(p_tiles[i]));
		if (it == by_cell_.end()) {
			continue;
		}
		Pickup *pickup = it->second;
		by_cell_.erase(it);
		if (pickup != nullptr && !pickup->is_consumed()) {
			remove_child(pickup);
			pickup->queue_free();
			active_count_--;
			removed++;
		}
	}
	return removed;
}

std::vector<Pickup *> PickupFactory::get_active_pickups() const {
	std::vector<Pickup *> result;
	for (int i = 0; i < get_child_count(); i++) {
		Pickup *pickup = Object::cast_to<Pickup>(get_child(i));
		if (pickup != nullptr && !pickup->is_consumed()) {
			result.push_back(pickup);
		}
	}
	return result;
}

void PickupFactory::on_pickup_collected(Object *p_participant, Pickup *p_pickup) {
	index_remove(p_pickup);
	active_count_--;
	emit_signal("pickup_collected", p_pickup, p_participant);
}
