#include "game/weapon_type.h"

#include <godot_cpp/core/class_db.hpp>

void WeaponType::_bind_methods() {
#define LV_BIND(name, variant, hint)                                                \
	ClassDB::bind_method(D_METHOD("set_" #name, "value"), &WeaponType::set_##name); \
	ClassDB::bind_method(D_METHOD("get_" #name), &WeaponType::get_##name);          \
	ADD_PROPERTY(PropertyInfo(Variant::variant, #name, PROPERTY_HINT_RANGE, hint), "set_" #name, "get_" #name);
	ClassDB::bind_method(D_METHOD("set_display_name", "value"), &WeaponType::set_display_name);
	ClassDB::bind_method(D_METHOD("get_display_name"), &WeaponType::get_display_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	ADD_GROUP("Disparo", "");
	LV_BIND(fire_interval, FLOAT, "0,3,0.01,suffix:s")
	LV_BIND(magazine_size, INT, "0,200,1") // 0 = infinitas
	LV_BIND(reload_time, FLOAT, "0,10,0.05,suffix:s")
	ADD_GROUP("Bala", "");
	LV_BIND(projectile_speed, FLOAT, "1,100,0.5,suffix:casillas/s")
	LV_BIND(range, FLOAT, "1,100,0.5,suffix:casillas")
	ClassDB::bind_method(D_METHOD("set_range_from_vision", "value"), &WeaponType::set_range_from_vision);
	ClassDB::bind_method(D_METHOD("get_range_from_vision"), &WeaponType::get_range_from_vision);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "range_from_vision"), "set_range_from_vision", "get_range_from_vision");
	LV_BIND(range_vision_factor, FLOAT, "0.1,3,0.05")
	LV_BIND(projectile_radius, FLOAT, "0.02,1,0.01,suffix:casillas")
#undef LV_BIND
	ClassDB::bind_method(D_METHOD("set_projectile_color", "value"), &WeaponType::set_projectile_color);
	ClassDB::bind_method(D_METHOD("get_projectile_color"), &WeaponType::get_projectile_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "projectile_color"), "set_projectile_color", "get_projectile_color");
}
