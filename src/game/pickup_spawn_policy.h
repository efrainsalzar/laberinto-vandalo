#pragma once

#include "core/pickup_spawning.h"

#include <godot_cpp/classes/resource.hpp>

using namespace godot;

// Patrón Strategy: decide CUÁNTOS objetos deben aparecer en cada momento.
// PickupSpawner pregunta a su política en cada frame; dónde aparecen lo decide él.
// Hoy: KeepDensityPolicy. Fase 5: el Director de IA tendrá su propia política
// (más objetos para quien va perdiendo, eventos, etc.) sin tocar PickupSpawner.
// Es un Resource para poder elegirla y ajustarla en el inspector. Ver docs/ARQUITECTURA.md §9.
class PickupSpawnPolicy : public Resource {
	GDCLASS(PickupSpawnPolicy, Resource)

public:
	PickupSpawnPolicy() { set_local_to_scene(true); } // cada spawner con su propio estado

	// Cantidad de objetos a crear ahora. La base no crea nada.
	virtual int compute_spawns(int p_active_count, double p_delta) { return 0; }
	// El spawner le dice el cupo de su categoría (escala con el mapa) y en cuánto reponerlo entero.
	virtual void configure(int p_target, float p_refill_time) {}

protected:
	static void _bind_methods() {}
};

// Mantener la densidad: si hay menos de `target_count` objetos, aparece uno cada `respawn_delay` s.
class KeepDensityPolicy : public PickupSpawnPolicy {
	GDCLASS(KeepDensityPolicy, PickupSpawnPolicy)

public:
	int compute_spawns(int p_active_count, double p_delta) override { return rule_.update(p_active_count, p_delta); }
	void configure(int p_target, float p_refill_time) override {
		rule_.target_count = p_target;
		if (p_refill_time > 0.0f) {
			rule_.respawn_delay = lv::core::refill_delay(p_target, p_refill_time);
		}
	}

	void set_target_count(int p_value) { rule_.target_count = p_value; }
	int get_target_count() const { return rule_.target_count; }
	void set_respawn_delay(float p_value) { rule_.respawn_delay = p_value; }
	float get_respawn_delay() const { return rule_.respawn_delay; }

protected:
	static void _bind_methods();

private:
	lv::core::KeepDensityRule rule_;
};
