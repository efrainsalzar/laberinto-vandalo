#include "game/weapon_component.h"

#include "game/participant.h"
#include "game/projectile_system.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>

void WeaponComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_weapon", "value"), &WeaponComponent::set_weapon);
	ClassDB::bind_method(D_METHOD("get_weapon"), &WeaponComponent::get_weapon);
	ClassDB::bind_method(D_METHOD("equip_special", "weapon"), &WeaponComponent::equip_special);
	ClassDB::bind_method(D_METHOD("clear_special"), &WeaponComponent::clear_special);
	ClassDB::bind_method(D_METHOD("get_special"), &WeaponComponent::get_special);
	ClassDB::bind_method(D_METHOD("has_special"), &WeaponComponent::has_special);
	ClassDB::bind_method(D_METHOD("get_special_ammo"), &WeaponComponent::get_special_ammo);
	ClassDB::bind_method(D_METHOD("get_active_weapon"), &WeaponComponent::get_active_weapon);
	ClassDB::bind_method(D_METHOD("is_ready"), &WeaponComponent::is_ready);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "weapon", PROPERTY_HINT_RESOURCE_TYPE, "WeaponType"), "set_weapon", "get_weapon");

	ADD_SIGNAL(MethodInfo("fired", PropertyInfo(Variant::VECTOR2, "direction")));
	ADD_SIGNAL(MethodInfo("weapon_changed", PropertyInfo(Variant::OBJECT, "weapon", PROPERTY_HINT_RESOURCE_TYPE, "WeaponType")));
	ADD_SIGNAL(MethodInfo("ammo_changed", PropertyInfo(Variant::INT, "ammo")));
}

void WeaponComponent::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		set_physics_process(false);
		return;
	}
	// Al ser eliminado pierde el arma especial (vuelve con la pistola).
	if (Participant *owner = Object::cast_to<Participant>(get_parent())) {
		if (LifeComponent *life = owner->get_life()) {
			life->connect("eliminated", callable_mp(this, &WeaponComponent::on_owner_eliminated));
		}
	}
}

void WeaponComponent::_physics_process(double p_delta) {
	const float dt = static_cast<float>(p_delta);
	base_state_.tick(dt);
	if (special_.is_valid()) {
		special_state_.tick(dt);
	}
}

void WeaponComponent::set_weapon(const Ref<WeaponType> &p_value) {
	weapon_ = p_value;
	base_state_.setup(weapon_.is_valid() ? weapon_->spec() : lv::core::WeaponSpec());
	if (special_.is_null()) {
		emit_signal("weapon_changed", weapon_);
	}
}

void WeaponComponent::equip_special(const Ref<WeaponType> &p_value) {
	if (p_value.is_null()) {
		clear_special();
		return;
	}
	special_ = p_value;
	special_state_.setup(special_->spec());
	emit_signal("weapon_changed", special_);
	emit_signal("ammo_changed", get_special_ammo());
}

void WeaponComponent::clear_special() {
	if (special_.is_null()) {
		return;
	}
	special_.unref();
	emit_signal("weapon_changed", weapon_);
	emit_signal("ammo_changed", 0);
}

int WeaponComponent::get_special_ammo() const {
	return special_.is_valid() ? special_state_.total_ammo() : 0;
}

void WeaponComponent::on_owner_eliminated(Object *) {
	clear_special();
}

float WeaponComponent::get_effective_range(const Participant *p_participant) const {
	const Ref<WeaponType> weapon = get_active_weapon();
	if (weapon.is_null()) {
		return 0.0f;
	}
	const StatsComponent *stats = p_participant != nullptr ? p_participant->get_stats() : nullptr;
	return lv::core::effective_range(weapon->spec(), stats != nullptr ? stats->get_vision() : 5.0f);
}

bool WeaponComponent::try_fire(Participant *p_participant, const Vector2 &p_direction) {
	const Ref<WeaponType> weapon = get_active_weapon();
	if (weapon.is_null() || p_participant == nullptr || p_direction.is_zero_approx() || !active_state().is_ready()) {
		return false;
	}
	ProjectileSystem *projectiles = ProjectileSystem::find(get_tree());
	if (projectiles == nullptr) {
		return false; // escena sin sistema de balas (p. ej. un sandbox): no se gasta el disparo
	}
	if (!active_state().try_fire()) {
		return false;
	}
	const Vector2 direction = p_direction.normalized();
	const Vector2 from = p_participant->get_global_position();
	const float range = get_effective_range(p_participant);
	const lv::core::WeaponSpec &spec = weapon->spec();
	if (spec.fire_mode == lv::core::FireMode::Beam) {
		projectiles->fire_beam(p_participant, from, direction, weapon, range);
	} else {
		lv::core::spread_directions(direction.x, direction.y, spec.pellets, spec.spread_deg, directions_);
		for (const lv::core::Direction &d : directions_) {
			projectiles->spawn(p_participant, from, Vector2(d.x, d.y), weapon, range);
		}
	}
	emit_signal("fired", direction);

	if (special_.is_valid()) {
		emit_signal("ammo_changed", get_special_ammo());
		if (special_state_.is_depleted()) {
			clear_special(); // sin munición: vuelve a la pistola
		}
	}
	return true;
}
