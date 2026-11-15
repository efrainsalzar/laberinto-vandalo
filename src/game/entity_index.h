#pragma once

#include "core/spatial_hash.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <unordered_map>

using namespace godot;

// Índice espacial de entidades en movimiento (participantes; luego bots, vándalos, proyectiles).
// Responde "¿quién está cerca de este punto?" revisando solo las cubetas cercanas (lv::core::SpatialHash).
//
// - Sigue automáticamente a los nodos de los grupos de `tracked_groups` (por defecto "participants"):
//   la membresía se sincroniza cada `sync_interval` s y las posiciones cada frame de física.
// - Hay uno por nivel; los sistemas lo encuentran con EntityIndex::find(get_tree()) (grupo "entity_index").
// Ver docs/ARQUITECTURA.md §17 y RENDIMIENTO.md (regla 7).
class EntityIndex : public Node {
	GDCLASS(EntityIndex, Node)

public:
	static EntityIndex *find(SceneTree *p_tree);

	void _ready() override;
	void _physics_process(double p_delta) override;

	TypedArray<Node2D> query_radius(const Vector2 &p_center, float p_radius) const;
	int get_tracked_count() const { return static_cast<int>(nodes_.size()); }

	void set_bucket_size(float p_value);
	float get_bucket_size() const { return bucket_size_; }

protected:
	static void _bind_methods();

private:
	float bucket_size_ = 64.0f;
	float sync_interval_ = 0.5f;
	float sync_timer_ = 0.0f;
	lv::core::SpatialHash hash_{ 64.0f };
	std::unordered_map<int, ObjectID> nodes_; // id interno -> nodo
	std::unordered_map<uint64_t, int> ids_; // instance id -> id interno
	int next_id_ = 1;

	void sync_membership();
};
