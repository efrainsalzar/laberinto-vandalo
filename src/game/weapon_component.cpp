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
	ClassDB::bind_method(D_METHOD("get_special_energy"), &WeaponComponent::get_special_energy);
	ClassDB::bind_method(D_METHOD("get_special_charge"), &WeaponComponent::get_special_charge);
	ClassDB::bind_method(D_METHOD("get_active_weapon"), &WeaponComponent::get_active_weapon);
	ClassDB::bind_method(D_METHOD("is_ready"), &WeaponComponent::is_ready);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "weapon", PROPERTY_HINT_RESOURCE_TYPE, "WeaponType"), "set_weapon", "get_weapon");

	ADD_SIGNAL(MethodInfo("fired", PropertyInfo(Variant::VECTOR2, "direction")));
	ADD_SIGNAL(MethodInfo("weapon_changed", PropertyInfo(Variant::OBJECT, "weapon", PROPERTY_HINT_RESOURCE_TYPE, "WeaponType")));
	ADD_SIGNAL(MethodInfo("charge_changed", PropertyInfo(Variant::FLOAT, "fraction")));
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
	const lv::core::WeaponSpec spec = weapon_.is_valid() ? weapon_->spec() : lv::core::WeaponSpec();
	base_state_.setup(spec);
	base_behavior_ = FireBehavior::create(spec.fire_mode);
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
	special_behavior_ = FireBehavior::create(special_->spec().fire_mode);
	emit_signal("weapon_changed", special_);
	emit_signal("charge_changed", get_special_charge());
}

void WeaponComponent::clear_special() {
	if (special_.is_null()) {
		return;
	}
	special_.unref();
	special_behavior_.reset();
	emit_signal("weapon_changed", weapon_);
	emit_signal("charge_changed", 0.0f);
}

int WeaponComponent::get_special_ammo() const {
	return special_.is_valid() ? special_state_.total_ammo() : 0;
}

float WeaponComponent::get_special_energy() const {
	return special_.is_valid() ? special_state_.energy() : -1.0f;
}

float WeaponComponent::get_special_charge() const {
	if (special_.is_null()) {
		return 0.0f;
	}
	const lv::core::WeaponSpec &spec = special_->spec();
	if (spec.energy > 0.0f) {
		return special_state_.energy() / spec.energy;
	}
	if (spec.ammo > 0) {
		return static_cast<float>(special_state_.total_ammo()) / static_cast<float>(spec.ammo);
	}
	return 1.0f;
}

void WeaponComponent::on_owner_eliminated(Object *) {
	trigger_down_ = false;
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

bool WeaponComponent::update_trigger(Participant *p_participant, bool p_down, const Vector2 &p_aim, float p_delta) {
	const lv::core::Trigger trigger = lv::core::next_trigger(trigger_down_, p_down);
	trigger_down_ = p_down;
	const bool special = special_.is_valid();
	FireBehavior *behavior = special ? special_behavior_.get() : base_behavior_.get();
	if (behavior == nullptr || p_participant == nullptr || p_aim.is_zero_approx() || trigger == lv::core::Trigger::Idle) {
		return false;
	}
	ProjectileSystem *projectiles = ProjectileSystem::find(get_tree());
	if (projectiles == nullptr) {
		return false; // escena sin sistema de balas (p. ej. un sandbox): no se gasta el disparo
	}
	FireContext context;
	context.shooter = p_participant;
	context.projectiles = projectiles;
	context.weapon = get_active_weapon();
	context.state = special ? &special_state_ : &base_state_;
	context.from = p_participant->get_global_position();
	context.direction = p_aim.normalized();
	context.range = get_effective_range(p_participant);
	context.delta = p_delta;
	if (!behavior->update(context, trigger)) {
		return false;
	}
	emit_signal("fired", context.direction);
	if (special) {
		emit_signal("charge_changed", get_special_charge());
		if (special_state_.is_depleted()) {
			clear_special(); // sin munición o batería: vuelve a la pistola
		}
	}
	return true;
}
