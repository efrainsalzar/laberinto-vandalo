#include "game/maze_profile.h"

#include <godot_cpp/core/class_db.hpp>

void MazeProfile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_display_name"), &MazeProfile::get_display_name);
	ClassDB::bind_method(D_METHOD("load_preset_values", "difficulty"), &MazeProfile::load_preset_values);
	ClassDB::bind_method(D_METHOD("set_difficulty", "value"), &MazeProfile::set_difficulty);
	ClassDB::bind_method(D_METHOD("get_difficulty"), &MazeProfile::get_difficulty);
	ClassDB::bind_method(D_METHOD("set_cells_width", "value"), &MazeProfile::set_cells_width);
	ClassDB::bind_method(D_METHOD("get_cells_width"), &MazeProfile::get_cells_width);
	ClassDB::bind_method(D_METHOD("set_cells_height", "value"), &MazeProfile::set_cells_height);
	ClassDB::bind_method(D_METHOD("get_cells_height"), &MazeProfile::get_cells_height);
	ClassDB::bind_method(D_METHOD("set_corridor_width", "value"), &MazeProfile::set_corridor_width);
	ClassDB::bind_method(D_METHOD("get_corridor_width"), &MazeProfile::get_corridor_width);
	ClassDB::bind_method(D_METHOD("set_algorithm", "value"), &MazeProfile::set_algorithm);
	ClassDB::bind_method(D_METHOD("get_algorithm"), &MazeProfile::get_algorithm);
	ClassDB::bind_method(D_METHOD("set_growing_tree_bias", "value"), &MazeProfile::set_growing_tree_bias);
	ClassDB::bind_method(D_METHOD("get_growing_tree_bias"), &MazeProfile::get_growing_tree_bias);
	ClassDB::bind_method(D_METHOD("set_loop_chance", "value"), &MazeProfile::set_loop_chance);
	ClassDB::bind_method(D_METHOD("get_loop_chance"), &MazeProfile::get_loop_chance);
	ClassDB::bind_method(D_METHOD("set_braid_chance", "value"), &MazeProfile::set_braid_chance);
	ClassDB::bind_method(D_METHOD("get_braid_chance"), &MazeProfile::get_braid_chance);
	ClassDB::bind_method(D_METHOD("set_vault_radius", "value"), &MazeProfile::set_vault_radius);
	ClassDB::bind_method(D_METHOD("get_vault_radius"), &MazeProfile::get_vault_radius);
	ClassDB::bind_method(D_METHOD("set_pickup_count", "value"), &MazeProfile::set_pickup_count);
	ClassDB::bind_method(D_METHOD("get_pickup_count"), &MazeProfile::get_pickup_count);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "difficulty", PROPERTY_HINT_ENUM, "Fácil:0,Normal:1,Difícil:2,Extremo:3,Personalizado:100"), "set_difficulty", "get_difficulty");
	ADD_GROUP("Personalizado", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cells_width", PROPERTY_HINT_RANGE, "2,120,1"), "set_cells_width", "get_cells_width");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cells_height", PROPERTY_HINT_RANGE, "2,120,1"), "set_cells_height", "get_cells_height");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "corridor_width", PROPERTY_HINT_RANGE, "1,8,1,suffix:casillas"), "set_corridor_width", "get_corridor_width");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "algorithm", PROPERTY_HINT_ENUM, "Backtracking,Growing Tree,Prim,Kruskal,Wilson,Hunt-and-Kill"), "set_algorithm", "get_algorithm");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "growing_tree_bias", PROPERTY_HINT_RANGE, "0,1,0.05"), "set_growing_tree_bias", "get_growing_tree_bias");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "loop_chance", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_loop_chance", "get_loop_chance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "braid_chance", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_braid_chance", "get_braid_chance");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "vault_radius", PROPERTY_HINT_RANGE, "0,12,1,suffix:casillas"), "set_vault_radius", "get_vault_radius");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "pickup_count", PROPERTY_HINT_RANGE, "0,500,1"), "set_pickup_count", "get_pickup_count");

	BIND_ENUM_CONSTANT(DIFFICULTY_EASY);
	BIND_ENUM_CONSTANT(DIFFICULTY_NORMAL);
	BIND_ENUM_CONSTANT(DIFFICULTY_HARD);
	BIND_ENUM_CONSTANT(DIFFICULTY_EXTREME);
	BIND_ENUM_CONSTANT(DIFFICULTY_CUSTOM);
	BIND_ENUM_CONSTANT(ALGORITHM_BACKTRACKER);
	BIND_ENUM_CONSTANT(ALGORITHM_GROWING_TREE);
	BIND_ENUM_CONSTANT(ALGORITHM_PRIM);
	BIND_ENUM_CONSTANT(ALGORITHM_KRUSKAL);
	BIND_ENUM_CONSTANT(ALGORITHM_WILSON);
	BIND_ENUM_CONSTANT(ALGORITHM_HUNT_AND_KILL);
}

// Los campos personalizados solo se muestran (y se guardan) en modo PERSONALIZADO.
void MazeProfile::_validate_property(PropertyInfo &p_property) const {
	static const char *CUSTOM_FIELDS[] = { "cells_width", "cells_height", "corridor_width", "algorithm", "growing_tree_bias",
		"loop_chance", "braid_chance", "vault_radius", "pickup_count" };
	if (difficulty_ == DIFFICULTY_CUSTOM) {
		return;
	}
	for (const char *field : CUSTOM_FIELDS) {
		if (p_property.name == StringName(field)) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
	}
}

void MazeProfile::set_difficulty(Difficulty p_value) {
	difficulty_ = p_value;
	notify_property_list_changed();
}

lv::core::MazeGrid::Options MazeProfile::to_options() const {
	if (difficulty_ == DIFFICULTY_CUSTOM) {
		return custom_;
	}
	return lv::core::maze_preset(static_cast<lv::core::MazeDifficulty>(difficulty_));
}

String MazeProfile::get_display_name() const {
	if (difficulty_ == DIFFICULTY_CUSTOM) {
		return "Personalizado";
	}
	return String::utf8(lv::core::maze_difficulty_name(static_cast<lv::core::MazeDifficulty>(difficulty_)));
}

int MazeProfile::get_pickup_count() const {
	if (difficulty_ == DIFFICULTY_CUSTOM) {
		return pickup_count_;
	}
	// Densidad constante: ~1 objeto cada 9 celdas lógicas.
	const lv::core::MazeGrid::Options o = to_options();
	return (o.cells_w * o.cells_h) / 9;
}

void MazeProfile::load_preset_values(Difficulty p_difficulty) {
	if (p_difficulty == DIFFICULTY_CUSTOM) {
		return;
	}
	custom_ = lv::core::maze_preset(static_cast<lv::core::MazeDifficulty>(p_difficulty));
	pickup_count_ = (custom_.cells_w * custom_.cells_h) / 9;
	notify_property_list_changed();
}