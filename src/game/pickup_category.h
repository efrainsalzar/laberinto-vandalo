#pragma once

#include "game/pickup_spawn_policy.h"
#include "game/pickup_type.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

// Una categoría de objetos como DATOS (.tres): puntos, power-ups y, más adelante, armas.
// Cada categoría tiene su propio cupo, su ritmo de reposición y cuánto dura un objeto en el piso,
// así unas no le quitan lugar a otras. Agregar una categoría = crear un archivo.
// Ejemplos: project/data/spawning/*.tres. Ver docs/ARQUITECTURA.md §20.
class PickupCategory : public Resource {
	GDCLASS(PickupCategory, Resource)

public:
	void set_id(const String &p_value) { id_ = p_value; }
	String get_id() const { return id_; }
	void set_types(const TypedArray<PickupType> &p_value) { types_ = p_value; }
	TypedArray<PickupType> get_types() const { return types_; }
	// Strategy: decide cuántos aparecen y cuándo (hoy KeepDensityPolicy; luego el Director de IA).
	void set_policy(const Ref<PickupSpawnPolicy> &p_value) { policy_ = p_value; }
	Ref<PickupSpawnPolicy> get_policy() const { return policy_; }

	// Cupo = presupuesto del mapa × share, entre min_count y max_count (max_count 0 = sin tope).
	void set_share(float p_value) { share_ = p_value; }
	float get_share() const { return share_; }
	void set_min_count(int p_value) { min_count_ = p_value; }
	int get_min_count() const { return min_count_; }
	void set_max_count(int p_value) { max_count_ = p_value; }
	int get_max_count() const { return max_count_; }
	// Segundos para reponer el cupo completo si se vaciara (la reposición escala con el mapa).
	void set_refill_time(float p_value) { refill_time_ = p_value; }
	float get_refill_time() const { return refill_time_; }
	// Al empezar la partida se coloca esta parte del cupo (0..1).
	void set_initial_fill(float p_value) { initial_fill_ = p_value; }
	float get_initial_fill() const { return initial_fill_; }

	// Segundos en el piso antes de desaparecer si nadie lo recoge (0 = para siempre).
	void set_lifetime(float p_value) { lifetime_ = p_value; }
	float get_lifetime() const { return lifetime_; }
	// Segundos antes de desaparecer en que el objeto parpadea.
	void set_warn_time(float p_value) { warn_time_ = p_value; }
	float get_warn_time() const { return warn_time_; }

protected:
	static void _bind_methods();

private:
	String id_ = "categoria";
	TypedArray<PickupType> types_;
	Ref<PickupSpawnPolicy> policy_;
	float share_ = 0.3f;
	int min_count_ = 1;
	int max_count_ = 0;
	float refill_time_ = 40.0f;
	float initial_fill_ = 1.0f;
	float lifetime_ = 0.0f;
	float warn_time_ = 5.0f;
};
