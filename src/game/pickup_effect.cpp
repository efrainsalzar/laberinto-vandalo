#include "game/pickup_effect.h"

#include "game/participant.h"

#include <godot_cpp/core/class_db.hpp>

void StatEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_stat", "value"), &StatEffect::set_stat);
	ClassDB::bind_method(D_METHOD("get_stat"), &StatEffect::get_stat);
	ClassDB::bind_method(D_METHOD("set_op", "value"), &StatEffect::set_op);
	ClassDB::bind_method(D_METHOD("get_op"), &StatEffect::get_op);
	ClassDB::bind_method(D_METHOD("set_value", "value"), &StatEffect::set_value);
	ClassDB::bind_method(D_METHOD("get_value"), &StatEffect::get_value);
	ClassDB::bind_method(D_METHOD("set_duration", "value"), &StatEffect::set_duration);
	ClassDB::bind_method(D_METHOD("get_duration"), &StatEffect::get_duration);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "stat", PROPERTY_HINT_ENUM, "Velocidad,Visión"), "set_stat", "get_stat");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "op", PROPERTY_HINT_ENUM, "Sumar,Multiplicar"), "set_op", "get_op");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value"), "set_value", "get_value");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "-1,120,0.5,suffix:s"), "set_duration", "get_duration");
}

void StatEffect::apply(Participant *p_participant, const String &p_source) const {
	if (StatsComponent *stats = p_participant != nullptr ? p_participant->get_stats() : nullptr) {
		stats->add_modifier(stat_, op_, value_, duration_, p_source);
	}
}

void HealEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_amount", "value"), &HealEffect::set_amount);
	ClassDB::bind_method(D_METHOD("get_amount"), &HealEffect::get_amount);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "amount", PROPERTY_HINT_RANGE, "1,500,1"), "set_amount", "get_amount");
}

bool HealEffect::is_useful(Participant *p_participant) const {
	const HealthComponent *health = p_participant != nullptr ? p_participant->get_health() : nullptr;
	return health != nullptr && !health->is_dead() && health->get_health() < health->get_max_health();
}

void HealEffect::apply(Participant *p_participant, const String &p_source) const {
	if (HealthComponent *health = p_participant != nullptr ? p_participant->get_health() : nullptr) {
		health->heal(amount_);
	}
}

void WeaponEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_weapon", "value"), &WeaponEffect::set_weapon);
	ClassDB::bind_method(D_METHOD("get_weapon"), &WeaponEffect::get_weapon);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "weapon", PROPERTY_HINT_RESOURCE_TYPE, "WeaponType"), "set_weapon", "get_weapon");
}

void WeaponEffect::apply(Participant *p_participant, const String &p_source) const {
	if (weapon_.is_null()) {
		return;
	}
	if (WeaponComponent *weapon = p_participant != nullptr ? p_participant->get_weapon() : nullptr) {
		weapon->equip_special(weapon_);
	}
}

void ShieldEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_kind", "value"), &ShieldEffect::set_kind);
	ClassDB::bind_method(D_METHOD("get_kind"), &ShieldEffect::get_kind);
	ClassDB::bind_method(D_METHOD("set_duration", "value"), &ShieldEffect::set_duration);
	ClassDB::bind_method(D_METHOD("get_duration"), &ShieldEffect::get_duration);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "kind", PROPERTY_HINT_ENUM, "Burbuja,Frontal,Espejo"), "set_kind", "get_kind");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "duration", PROPERTY_HINT_RANGE, "0,120,0.5,suffix:s"), "set_duration", "get_duration"); // 0 = por defecto
}

void ShieldEffect::apply(Participant *p_participant, const String &p_source) const {
	if (ShieldComponent *shields = p_participant != nullptr ? p_participant->get_shields() : nullptr) {
		shields->add_shield(kind_, duration_);
	}
}
