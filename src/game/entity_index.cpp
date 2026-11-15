#include "game/entity_index.h"

#include "core/perf.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <vector>

namespace {
const char *GROUP_SELF = "entity_index";
const char *GROUP_TRACKED = "participants";
} // namespace

void EntityIndex::_bind_methods() {
	ClassDB::bind_method(D_METHOD("query_radius", "center", "radius"), &EntityIndex::query_radius);
	ClassDB::bind_method(D_METHOD("get_tracked_count"), &EntityIndex::get_tracked_count);
	ClassDB::bind_method(D_METHOD("set_bucket_size", "value"), &EntityIndex::set_bucket_size);
	ClassDB::bind_method(D_METHOD("get_bucket_size"), &EntityIndex::get_bucket_size);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bucket_size", PROPERTY_HINT_RANGE, "8,512,1,suffix:px"), "set_bucket_size", "get_bucket_size");
}

EntityIndex *EntityIndex::find(SceneTree *p_tree) {
	if (p_tree == nullptr) {
		return nullptr;
	}
	return Object::cast_to<EntityIndex>(p_tree->get_first_node_in_group(GROUP_SELF));
}

void EntityIndex::set_bucket_size(float p_value) {
	bucket_size_ = p_value;
	hash_ = lv::core::SpatialHash(p_value);
	ids_.clear();
	nodes_.clear();
}

void EntityIndex::_ready() {
	add_to_group(GROUP_SELF);
}

void EntityIndex::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	lv::core::PerfScope perf("entities.update");
	sync_timer_ -= static_cast<float>(p_delta);
	if (sync_timer_ <= 0.0f) {
		sync_timer_ = sync_interval_;
		sync_membership();
	}
	std::vector<int> gone;
	for (const auto &entry : nodes_) {
		Node2D *node = Object::cast_to<Node2D>(ObjectDB::get_instance(entry.second));
		if (node == nullptr) {
			gone.push_back(entry.first);
			continue;
		}
		const Vector2 p = node->get_global_position();
		hash_.move(entry.first, p.x, p.y); // casi gratis si no cambió de cubeta
	}
	for (int id : gone) {
		hash_.remove(id);
		nodes_.erase(id);
	}
}

void EntityIndex::sync_membership() {
	const TypedArray<Node> tracked = get_tree()->get_nodes_in_group(GROUP_TRACKED);
	for (int i = 0; i < tracked.size(); i++) {
		Node2D *node = Object::cast_to<Node2D>(tracked[i]);
		if (node == nullptr) {
			continue;
		}
		const uint64_t instance = node->get_instance_id();
		if (ids_.count(instance) != 0) {
			continue;
		}
		const int id = next_id_++;
		ids_[instance] = id;
		nodes_[id] = ObjectID(instance);
		const Vector2 p = node->get_global_position();
		hash_.insert(id, p.x, p.y);
	}
}

TypedArray<Node2D> EntityIndex::query_radius(const Vector2 &p_center, float p_radius) const {
	TypedArray<Node2D> result;
	std::vector<int> ids;
	hash_.query_radius(p_center.x, p_center.y, p_radius, ids);
	for (int id : ids) {
		const auto it = nodes_.find(id);
		if (it == nodes_.end()) {
			continue;
		}
		if (Node2D *node = Object::cast_to<Node2D>(ObjectDB::get_instance(it->second))) {
			result.push_back(node);
		}
	}
	return result;
}
