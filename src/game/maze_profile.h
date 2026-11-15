#pragma once

#include "core/maze_grid.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Perfil de generación de un mapa como DATOS (.tres): nivel de dificultad predefinido o
// valores personalizados. Es el comienzo de `MapConfig` (DISENO_MAPA §1).
//
// - difficulty = FACIL/NORMAL/DIFICIL/EXTREMO → usa lv::core::maze_preset (fuente única de los valores).
// - difficulty = PERSONALIZADO → usa los campos de este recurso.
// Ver docs/ARQUITECTURA.md §12.
class MazeProfile : public Resource {
	GDCLASS(MazeProfile, Resource)

public:
	enum Difficulty {
		DIFFICULTY_EASY = static_cast<int>(lv::core::MazeDifficulty::Easy),
		DIFFICULTY_NORMAL = static_cast<int>(lv::core::MazeDifficulty::Normal),
		DIFFICULTY_HARD = static_cast<int>(lv::core::MazeDifficulty::Hard),
		DIFFICULTY_EXTREME = static_cast<int>(lv::core::MazeDifficulty::Extreme),
		DIFFICULTY_CUSTOM = 100,
	};
	enum Algorithm {
		ALGORITHM_BACKTRACKER = static_cast<int>(lv::core::MazeAlgorithm::RecursiveBacktracker),
		ALGORITHM_GROWING_TREE = static_cast<int>(lv::core::MazeAlgorithm::GrowingTree),
		ALGORITHM_PRIM = static_cast<int>(lv::core::MazeAlgorithm::Prim),
		ALGORITHM_KRUSKAL = static_cast<int>(lv::core::MazeAlgorithm::Kruskal),
		ALGORITHM_WILSON = static_cast<int>(lv::core::MazeAlgorithm::Wilson),
		ALGORITHM_HUNT_AND_KILL = static_cast<int>(lv::core::MazeAlgorithm::HuntAndKill),
	};

	// Opciones efectivas (preset o personalizadas). La semilla la pone Maze.
	lv::core::MazeGrid::Options to_options() const;
	String get_display_name() const;

	void set_difficulty(Difficulty p_value);
	Difficulty get_difficulty() const { return difficulty_; }
	void set_cells_width(int p_value) { custom_.cells_w = p_value; }
	int get_cells_width() const { return custom_.cells_w; }
	void set_cells_height(int p_value) { custom_.cells_h = p_value; }
	int get_cells_height() const { return custom_.cells_h; }
	void set_corridor_width(int p_value) { custom_.corridor_width = p_value; }
	int get_corridor_width() const { return custom_.corridor_width; }
	void set_algorithm(Algorithm p_value) { custom_.algorithm = static_cast<lv::core::MazeAlgorithm>(p_value); }
	Algorithm get_algorithm() const { return static_cast<Algorithm>(custom_.algorithm); }
	void set_growing_tree_bias(float p_value) { custom_.growing_tree_bias = p_value; }
	float get_growing_tree_bias() const { return custom_.growing_tree_bias; }
	void set_loop_chance(float p_value) { custom_.loop_chance = p_value; }
	float get_loop_chance() const { return custom_.loop_chance; }
	void set_braid_chance(float p_value) { custom_.braid_chance = p_value; }
	float get_braid_chance() const { return custom_.braid_chance; }
	void set_vault_radius(int p_value) { custom_.vault_radius = p_value; }
	int get_vault_radius() const { return custom_.vault_radius; }
	void set_pickup_count(int p_value) { pickup_count_ = p_value; }
	int get_pickup_count() const;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

private:
	Difficulty difficulty_ = DIFFICULTY_NORMAL;
	lv::core::MazeGrid::Options custom_ = lv::core::maze_preset(lv::core::MazeDifficulty::Normal);
	int pickup_count_ = 30; // solo en PERSONALIZADO; los niveles escalan con el tamaño
};

VARIANT_ENUM_CAST(MazeProfile::Difficulty);
VARIANT_ENUM_CAST(MazeProfile::Algorithm);
