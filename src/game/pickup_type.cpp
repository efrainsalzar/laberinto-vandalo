#include "game/pickup_type.h"

#include "game/participant.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

void PickupType::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_id", "value"), &PickupType::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &PickupType::get_id);
	ClassDB::bind_method(D_METHOD("set_display_name", "value"), &PickupType::set_display_name);
	ClassDB::bind_method(D_METHOD("get_display_name"), &PickupType::get_display_name);
	ClassDB::bind_method(D_METHOD("set_popup_title", "value"), &PickupType::set_popup_title);
	ClassDB::bind_method(D_METHOD("get_popup_title"), &PickupType::get_popup_title);
	ClassDB::bind_method(D_METHOD("set_color", "value"), &PickupType::set_color);
	ClassDB::bind_method(D_METHOD("get_color"), &PickupType::get_color);
	ClassDB::bind_method(D_METHOD("set_radius", "value"), &PickupType::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &PickupType::get_radius);
	ClassDB::bind_method(D_METHOD("set_icon", "value"), &PickupType::set_icon);
	ClassDB::bind_method(D_METHOD("get_icon"), &PickupType::get_icon);
	ClassDB::bind_method(D_METHOD("set_weight", "value"), &PickupType::set_weight);
	ClassDB::bind_method(D_METHOD("get_weight"), &PickupType::get_weight);
	ClassDB::bind_method(D_METHOD("set_effects", "value"), &PickupType::set_effects);
	ClassDB::bind_method(D_METHOD("get_effects"), &PickupType::get_effects);

	ClassDB::bind_method(D_METHOD("set_shape", "value"), &PickupType::set_shape);
	ClassDB::bind_method(D_METHOD("get_shape"), &PickupType::get_shape);
	ClassDB::bind_method(D_METHOD("set_symbol", "value"), &PickupType::set_symbol);
	ClassDB::bind_method(D_METHOD("get_symbol"), &PickupType::get_symbol);

	BIND_ENUM_CONSTANT(SHAPE_CIRCLE);
	BIND_ENUM_CONSTANT(SHAPE_DIAMOND);
	BIND_ENUM_CONSTANT(SHAPE_SQUARE);
	BIND_ENUM_CONSTANT(SHAPE_HEXAGON);
	BIND_ENUM_CONSTANT(SHAPE_RING);
	BIND_ENUM_CONSTANT(SHAPE_FLASK);
	BIND_ENUM_CONSTANT(SYMBOL_NONE);
	BIND_ENUM_CONSTANT(SYMBOL_SPEED);
	BIND_ENUM_CONSTANT(SYMBOL_VISION);
	BIND_ENUM_CONSTANT(SYMBOL_BOLT);
	BIND_ENUM_CONSTANT(SYMBOL_SPREAD);
	BIND_ENUM_CONSTANT(SYMBOL_BEAM);
	BIND_ENUM_CONSTANT(SYMBOL_BLAST);
	BIND_ENUM_CONSTANT(SYMBOL_CROSS);
	BIND_ENUM_CONSTANT(SYMBOL_BUBBLE);
	BIND_ENUM_CONSTANT(SYMBOL_ARC);
	BIND_ENUM_CONSTANT(SYMBOL_MIRROR);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "id"), "set_id", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "popup_title"), "set_popup_title", "get_popup_title");
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "1,32,0.5,suffix:px"), "set_radius", "get_radius");
	ADD_GROUP("Aspecto", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "shape", PROPERTY_HINT_ENUM, "Círculo (stat),Rombo (power-up),Cuadrado (arma),Hexágono (mapa),Anillo (portal),Frasco (poción)"), "set_shape", "get_shape");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "symbol", PROPERTY_HINT_ENUM, "Ninguno,Velocidad,Visión,Rayo,Abanico (escopeta),Haz (láser),Explosión (bazuca),Cruz (curar),Burbuja (escudo),Arco (escudo frontal),Espejo (escudo)"), "set_symbol", "get_symbol");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "icon", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_icon", "get_icon");
	ADD_GROUP("Reglas", "");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight", PROPERTY_HINT_RANGE, "0,100,0.1"), "set_weight", "get_weight");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "effects", PROPERTY_HINT_TYPE_STRING,
						 UtilityFunctions::str(Variant::OBJECT) + "/" + UtilityFunctions::str(PROPERTY_HINT_RESOURCE_TYPE) + ":PickupEffect"),
			"set_effects", "get_effects");
}

bool PickupType::is_useful_for(Participant *p_participant) const {
	if (effects_.is_empty()) {
		return true;
	}
	for (int i = 0; i < effects_.size(); i++) {
		const Ref<PickupEffect> effect = effects_[i];
		if (effect.is_valid() && effect->is_useful(p_participant)) {
			return true;
		}
	}
	return false;
}

void PickupType::apply_to(Participant *p_participant) const {
	const String source = "pickup:" + id_;
	for (int i = 0; i < effects_.size(); i++) {
		const Ref<PickupEffect> effect = effects_[i];
		if (effect.is_valid()) {
			effect->apply(p_participant, source);
		}
	}
}
