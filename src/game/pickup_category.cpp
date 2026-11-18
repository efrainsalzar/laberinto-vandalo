#include "game/pickup_category.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

void PickupCategory::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_id", "value"), &PickupCategory::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &PickupCategory::get_id);
	ClassDB::bind_method(D_METHOD("set_types", "value"), &PickupCategory::set_types);
	ClassDB::bind_method(D_METHOD("get_types"), &PickupCategory::get_types);
	ClassDB::bind_method(D_METHOD("set_policy", "value"), &PickupCategory::set_policy);
	ClassDB::bind_method(D_METHOD("get_policy"), &PickupCategory::get_policy);
	ClassDB::bind_method(D_METHOD("set_share", "value"), &PickupCategory::set_share);
	ClassDB::bind_method(D_METHOD("get_share"), &PickupCategory::get_share);
	ClassDB::bind_method(D_METHOD("set_min_count", "value"), &PickupCategory::set_min_count);
	ClassDB::bind_method(D_METHOD("get_min_count"), &PickupCategory::get_min_count);
	ClassDB::bind_method(D_METHOD("set_max_count", "value"), &PickupCategory::set_max_count);
	ClassDB::bind_method(D_METHOD("get_max_count"), &PickupCategory::get_max_count);
	ClassDB::bind_method(D_METHOD("set_refill_time", "value"), &PickupCategory::set_refill_time);
	ClassDB::bind_method(D_METHOD("get_refill_time"), &PickupCategory::get_refill_time);
	ClassDB::bind_method(D_METHOD("set_initial_fill", "value"), &PickupCategory::set_initial_fill);
	ClassDB::bind_method(D_METHOD("get_initial_fill"), &PickupCategory::get_initial_fill);
	ClassDB::bind_method(D_METHOD("set_lifetime", "value"), &PickupCategory::set_lifetime);
	ClassDB::bind_method(D_METHOD("get_lifetime"), &PickupCategory::get_lifetime);
	ClassDB::bind_method(D_METHOD("set_warn_time", "value"), &PickupCategory::set_warn_time);
	ClassDB::bind_method(D_METHOD("get_warn_time"), &PickupCategory::get_warn_time);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "id"), "set_id", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "types", PROPERTY_HINT_TYPE_STRING,
						 UtilityFunctions::str(Variant::OBJECT) + "/" + UtilityFunctions::str(PROPERTY_HINT_RESOURCE_TYPE) + ":PickupType"),
			"set_types", "get_types");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "policy", PROPERTY_HINT_RESOURCE_TYPE, "PickupSpawnPolicy"), "set_policy", "get_policy");
	ADD_GROUP("Cupo", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "share", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_share", "get_share");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "min_count", PROPERTY_HINT_RANGE, "0,100,1"), "set_min_count", "get_min_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_count", PROPERTY_HINT_RANGE, "0,500,1"), "set_max_count", "get_max_count");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refill_time", PROPERTY_HINT_RANGE, "0,600,1,suffix:s"), "set_refill_time", "get_refill_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "initial_fill", PROPERTY_HINT_RANGE, "0,1,0.05"), "set_initial_fill", "get_initial_fill");
	ADD_GROUP("En el piso", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "lifetime", PROPERTY_HINT_RANGE, "0,600,1,suffix:s"), "set_lifetime", "get_lifetime");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "warn_time", PROPERTY_HINT_RANGE, "0,30,0.5,suffix:s"), "set_warn_time", "get_warn_time");
}
