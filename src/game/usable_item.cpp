#include "game/usable_item.h"

#include <godot_cpp/core/class_db.hpp>

void UsableItem::_bind_methods() {
#define LV_BIND_RAW(name)                                                           \
	ClassDB::bind_method(D_METHOD("set_" #name, "value"), &UsableItem::set_##name); \
	ClassDB::bind_method(D_METHOD("get_" #name), &UsableItem::get_##name);
#define LV_BIND(name, variant, hint) \
	LV_BIND_RAW(name)                \
	ADD_PROPERTY(PropertyInfo(Variant::variant, #name, PROPERTY_HINT_RANGE, hint), "set_" #name, "get_" #name);

	LV_BIND_RAW(display_name)
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	LV_BIND_RAW(short_name)
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "short_name"), "set_short_name", "get_short_name");
	LV_BIND_RAW(kind)
	ADD_PROPERTY(PropertyInfo(Variant::INT, "kind", PROPERTY_HINT_ENUM, "Bomba,Mina"), "set_kind", "get_kind");
	LV_BIND(max_carry, INT, "1,10,1")

	ADD_GROUP("Tiempos", "");
	LV_BIND(fuse, FLOAT, "0.1,10,0.1,suffix:s") // bomba
	LV_BIND(arm_time, FLOAT, "0,10,0.1,suffix:s") // mina
	LV_BIND(trigger_radius, FLOAT, "0.1,3,0.05,suffix:casillas") // mina

	ADD_GROUP("Explosión", "");
	LV_BIND(explosion_radius, FLOAT, "0.5,10,0.1,suffix:casillas")
	LV_BIND(damage, FLOAT, "0,500,0.5")
	LV_BIND(damage_edge, FLOAT, "0,500,0.5")
	LV_BIND_RAW(color)
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
#undef LV_BIND
#undef LV_BIND_RAW

	BIND_ENUM_CONSTANT(KIND_BOMB);
	BIND_ENUM_CONSTANT(KIND_MINE);
}
