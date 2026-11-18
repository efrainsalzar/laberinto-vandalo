#pragma once

#include "core/pickup_spawning.h"
#include "core/random.h"
#include "game/pickup_category.h"
#include "game/pickup_factory.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <memory>
#include <vector>

using namespace godot;

class Maze;

// Hace que los objetos sean "infinitos" y que roten: coloca los iniciales, repone los que faltan
// y retira los que nadie recoge a tiempo.
//
// - Por CATEGORÍAS (PickupCategory, datos): puntos, power-ups, luego armas. Cada una con su cupo
//   (escala con el `budget` del mapa), su ritmo y su vida en el piso.
// - CUÁNTOS y CUÁNDO: la `policy` de cada categoría (Strategy: PickupSpawnPolicy).
// - DÓNDE: lv::core::pick_spawn_cell, nunca a la vista de un participante (grupo "participants").
// - CUÁNDO DESAPARECE: lv::core::ExpiryQueue (aviso → parpadeo → caduca).
// - CÓMO se crean: PickupFactory (Factory).
// Emite `pickup_spawned` y `pickup_expired` (Observer). Ver docs/ARQUITECTURA.md §9 y §20.
class PickupSpawner : public Node {
	GDCLASS(PickupSpawner, Node)

public:
	void _process(double p_delta) override;

	// Coloca los objetos iniciales y empieza a reponer. Llamar tras generar el laberinto.
	void start(Maze *p_maze, const TypedArray<Vector2i> &p_bases, int p_seed);
	void stop() { maze_ = nullptr; }

	void set_factory_path(const NodePath &p_value) { factory_path_ = p_value; }
	NodePath get_factory_path() const { return factory_path_; }
	void set_categories(const TypedArray<PickupCategory> &p_value) { categories_ = p_value; }
	TypedArray<PickupCategory> get_categories() const { return categories_; }
	// Presupuesto de objetos del mapa (MazeProfile.pickup_count): cada categoría toma su parte.
	void set_budget(int p_value) { budget_ = p_value; }
	int get_budget() const { return budget_; }
	void set_observer_margin(float p_value) { cell_options_.observer_margin = p_value; }
	float get_observer_margin() const { return cell_options_.observer_margin; }

	// Cupo y objetos activos de una categoría (HUD de depuración, tests).
	int get_category_target(int p_index) const;
	int get_category_active(int p_index) const;

protected:
	static void _bind_methods();

private:
	struct CategoryState {
		Ref<PickupCategory> category;
		Ref<PickupSpawnPolicy> policy; // copia propia: cada partida con su reloj
		int target = 0;
		int active = 0;
	};

	NodePath factory_path_;
	TypedArray<PickupCategory> categories_;
	int budget_ = 30;
	lv::core::SpawnCellOptions cell_options_;

	Maze *maze_ = nullptr;
	PickupFactory *factory_ = nullptr;
	std::vector<lv::core::Cell> bases_;
	std::unique_ptr<lv::core::Rng> rng_;
	std::vector<CategoryState> states_;
	lv::core::ExpiryQueue expiry_;
	double now_ = 0.0;
	int generation_ = 0; // descarta avisos de objetos de una partida anterior

	Pickup *spawn_one(int p_category);
	void track(Pickup *p_pickup, int p_category, double p_lifetime);
	void process_expiry();
	void on_pickup_exiting(int p_category, int p_generation);
	void on_map_changed(const TypedArray<Vector2i> &p_tiles, bool p_now_wall);
};
