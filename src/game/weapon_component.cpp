#include "game/weapon_component.h"

#include "game/participant.h"
#include "game/projectile_system.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

void WeaponComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_weapon", "value"), &WeaponComponent::set_weapon);
	ClassDB::bind_method(D_METHOD("get_weapon"), &WeaponComponent::get_weapon);
	ClassDB::bind_method(D_METHOD("is_ready"), &WeaponComponent::is_ready);
	ClassDB::bind_method(D_METHOD("get_ammo"), &WeaponComponent::get_ammo);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "weapon", PROPERTY_HINT_RESOURCE_TYPE, "WeaponType"), "set_weapon", "get_weapon");

	ADD_SIGNAL(MethodInfo("fired", PropertyInfo(Variant::VECTOR2, "direction")));
	ADD_SIGNAL(MethodInfo("weapon_changed", PropertyInfo(Variant::OBJECT, "weapon", PROPERTY_HINT_RESOURCE_TYPE, "WeaponType")));
}

void WeaponComponent::_ready() {
	set_physics_process(!Engine::get_singleton()->is_editor_hint());
}

void WeaponComponent::_physics_process(double p_delta) {
	state_.tick(static_cast<float>(p_delta));
}

void WeaponComponent::set_weapon(const Ref<WeaponType> &p_value) {
	weapon_ = p_value;
	state_.setup(weapon_.is_valid() ? weapon_->spec() : lv::core::WeaponSpec());
	emit_signal("weapon_changed", weapon_);
}

float WeaponComponent::get_effective_range(const Participant *p_participant) const {
	if (weapon_.is_null()) {
		return 0.0f;
	}
	const StatsComponent *stats = p_participant != nullptr ? p_participant->get_stats() : nullptr;
	return lv::core::effective_range(weapon_->spec(), stats != nullptr ? stats->get_vision() : 5.0f);
}

bool WeaponComponent::try_fire(Participant *p_participant, const Vector2 &p_direction) {
	if (weapon_.is_null() || p_participant == nullptr || p_direction.is_zero_approx() || !state_.is_ready()) {
		return false;
	}
	ProjectileSystem *projectiles = ProjectileSystem::find(get_tree());
	if (projectiles == nullptr) {
		return false; // escena sin sistema de balas (p. ej. un sandbox): no se gasta el disparo
	}
	if (!state_.try_fire()) {
		return false;
	}
	const Vector2 direction = p_direction.normalized();
	projectiles->spawn(p_participant, p_participant->get_global_position(), direction, weapon_, get_effective_range(p_participant));
	emit_signal("fired", direction);
	return true;
}
