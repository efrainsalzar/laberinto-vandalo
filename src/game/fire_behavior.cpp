#include "game/fire_behavior.h"

#include "game/projectile_system.h"

std::unique_ptr<FireBehavior> FireBehavior::create(lv::core::FireMode p_mode) {
	switch (p_mode) {
		case lv::core::FireMode::HeldBeam:
			return std::make_unique<HeldBeamFire>();
		case lv::core::FireMode::Projectile:
		default:
			return std::make_unique<ProjectileFire>();
	}
}

bool ProjectileFire::update(const FireContext &p_context, lv::core::Trigger p_trigger) {
	if (!lv::core::is_trigger_down(p_trigger) || !p_context.state->try_fire()) {
		return false;
	}
	const lv::core::WeaponSpec &spec = p_context.weapon->spec();
	lv::core::spread_directions(p_context.direction.x, p_context.direction.y, spec.pellets, spec.spread_deg, directions_);
	for (const lv::core::Direction &d : directions_) {
		p_context.projectiles->spawn(p_context.shooter, p_context.from, Vector2(d.x, d.y), p_context.weapon, p_context.range);
	}
	return true;
}

bool HeldBeamFire::update(const FireContext &p_context, lv::core::Trigger p_trigger) {
	if (!lv::core::is_trigger_down(p_trigger) || !p_context.state->drain(p_context.delta)) {
		return false; // suelto o sin batería: el rayo se apaga solo (ProjectileSystem lo quita si no se renueva)
	}
	p_context.projectiles->hold_beam(p_context.shooter, p_context.from, p_context.direction, p_context.weapon, p_context.range, p_context.delta);
	return true;
}
