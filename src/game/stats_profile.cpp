#include "game/stats_profile.h"

#include <godot_cpp/core/class_db.hpp>

void StatsProfile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_speed", "value"), &StatsProfile::set_speed);
	ClassDB::bind_method(D_METHOD("get_speed"), &StatsProfile::get_speed);
	ClassDB::bind_method(D_METHOD("set_vision", "value"), &StatsProfile::set_vision);
	ClassDB::bind_method(D_METHOD("get_vision"), &StatsProfile::get_vision);
	ClassDB::bind_method(D_METHOD("set_speed_bonus_max", "value"), &StatsProfile::set_speed_bonus_max);
	ClassDB::bind_method(D_METHOD("get_speed_bonus_max"), &StatsProfile::get_speed_bonus_max);
	ClassDB::bind_method(D_METHOD("set_vision_bonus_max", "value"), &StatsProfile::set_vision_bonus_max);
	ClassDB::bind_method(D_METHOD("get_vision_bonus_max"), &StatsProfile::get_vision_bonus_max);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed", PROPERTY_HINT_RANGE, "20,600,1,suffix:px/s"), "set_speed", "get_speed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vision", PROPERTY_HINT_RANGE, "1,20,0.5,suffix:casillas"), "set_vision", "get_vision");
	ADD_GROUP("Tope de mejoras permanentes", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_bonus_max", PROPERTY_HINT_RANGE, "-1,600,1,suffix:px/s"), "set_speed_bonus_max", "get_speed_bonus_max");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vision_bonus_max", PROPERTY_HINT_RANGE, "-1,20,0.25,suffix:casillas"), "set_vision_bonus_max", "get_vision_bonus_max");
}
