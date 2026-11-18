#include "game/weapon_type.h"

#include <godot_cpp/core/class_db.hpp>

void WeaponType::_bind_methods() {
#define LV_BIND_RAW(name)                                                           \
	ClassDB::bind_method(D_METHOD("set_" #name, "value"), &WeaponType::set_##name); \
	ClassDB::bind_method(D_METHOD("get_" #name), &WeaponType::get_##name);
#define LV_BIND(name, variant, hint) \
	LV_BIND_RAW(name)                \
	ADD_PROPERTY(PropertyInfo(Variant::variant, #name, PROPERTY_HINT_RANGE, hint), "set_" #name, "get_" #name);

	LV_BIND_RAW(display_name)
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	LV_BIND_RAW(short_name)
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "short_name"), "set_short_name", "get_short_name"); // para el HUD

	ADD_GROUP("Disparo", "");
	LV_BIND_RAW(fire_mode)
	ADD_PROPERTY(PropertyInfo(Variant::INT, "fire_mode", PROPERTY_HINT_ENUM, "Proyectil,Rayo sostenido"), "set_fire_mode", "get_fire_mode");
	LV_BIND(fire_interval, FLOAT, "0,3,0.01,suffix:s")
	LV_BIND(ammo, INT, "0,500,1") // 0 = infinita
	LV_BIND(energy, FLOAT, "0,60,0.1,suffix:s") // batería de los modos sostenidos; 0 = infinita
	LV_BIND(magazine_size, INT, "0,200,1") // 0 = sin cargador
	LV_BIND(reload_time, FLOAT, "0,10,0.05,suffix:s")
	LV_BIND(pellets, INT, "1,24,1")
	LV_BIND(spread_deg, FLOAT, "0,180,0.5,suffix:grados")

	ADD_GROUP("Daño", "");
	LV_BIND(damage, FLOAT, "0,500,0.5") // por bala/perdigón; rayo: por segundo; explosión: en el centro
	LV_BIND(damage_edge, FLOAT, "0,500,0.5") // explosión: en el borde del radio

	ADD_GROUP("Alcance", "");
	LV_BIND_RAW(range_from_vision)
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "range_from_vision"), "set_range_from_vision", "get_range_from_vision");
	LV_BIND(range_vision_factor, FLOAT, "0.1,3,0.05")
	LV_BIND(range, FLOAT, "1,100,0.5,suffix:casillas")

	ADD_GROUP("Bala", "");
	LV_BIND(projectile_speed, FLOAT, "1,200,0.5,suffix:casillas/s")
	LV_BIND(projectile_radius, FLOAT, "0.02,1,0.01,suffix:casillas")
	LV_BIND(explosion_radius, FLOAT, "0,10,0.1,suffix:casillas") // 0 = impacto directo
	LV_BIND_RAW(projectile_color)
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "projectile_color"), "set_projectile_color", "get_projectile_color");
#undef LV_BIND
#undef LV_BIND_RAW

	BIND_ENUM_CONSTANT(FIRE_PROJECTILE);
	BIND_ENUM_CONSTANT(FIRE_HELD_BEAM);
}
