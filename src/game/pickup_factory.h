#pragma once

#include "game/pickup.h"
#include "game/pickup_type.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

using namespace godot;

class Maze;

// Fábrica de objetos recolectables (patrón Factory) y contenedor de los objetos del nivel.
//
// - create(): instancia `pickup_scene` y le asigna un PickupType (los datos).
// - populate(): usa lv::core::place_pickups para decidir dónde y de qué tipo (según `weight`)
//   y crea todos los objetos como hijos de este nodo.
// - Reenvía el `collected` de cada objeto como `pickup_collected` (Observer): la interfaz
//   y otros sistemas se conectan una sola vez aquí.
// Ver docs/ARQUITECTURA.md §8.
class PickupFactory : public Node2D {
	GDCLASS(PickupFactory, Node2D)

public:
	Pickup *create(const Ref<PickupType> &p_type, const Vector2 &p_position);
	// Borra los objetos actuales y coloca nuevos en el laberinto. Devuelve cuántos creó.
	int populate(Maze *p_maze, const TypedArray<Vector2i> &p_bases, int p_seed);
	void clear_pickups();

	void set_pickup_scene(const Ref<PackedScene> &p_value) { pickup_scene_ = p_value; }
	Ref<PackedScene> get_pickup_scene() const { return pickup_scene_; }
	void set_types(const TypedArray<PickupType> &p_value) { types_ = p_value; }
	TypedArray<PickupType> get_types() const { return types_; }
	void set_count(int p_value) { count_ = p_value; }
	int get_count() const { return count_; }
	void set_min_spacing(int p_value) { min_spacing_ = p_value; }
	int get_min_spacing() const { return min_spacing_; }
	void set_base_clearance(int p_value) { base_clearance_ = p_value; }
	int get_base_clearance() const { return base_clearance_; }

protected:
	static void _bind_methods();

private:
	Ref<PackedScene> pickup_scene_;
	TypedArray<PickupType> types_;
	int count_ = 30;
	int min_spacing_ = 4;
	int base_clearance_ = 6;

	void on_pickup_collected(Object *p_participant, Pickup *p_pickup);
};
