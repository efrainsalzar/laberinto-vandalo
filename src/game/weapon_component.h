#pragma once

#include "core/weapon.h"
#include "game/weapon_type.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/vector2.hpp>

using namespace godot;

class Participant;

// El arma que lleva un participante, como componente hijo (composición, igual que StatsComponent y
// LifeComponent). Envuelve a lv::core::WeaponState (State: lista / enfriando / recargando).
//
// - Participant::execute_command le pasa la orden de disparo (ParticipantCommand.fire + aim).
// - Si el arma puede (cadencia, munición), pide la bala a ProjectileSystem (grupo "projectile_system").
// - Señal fired(direction) para la vista (sonido, destello) cuando lleguen. No dibuja.
// `weapon` empieza con la pistola (pistol.tres); las armas temporales del mapa la cambiarán (siguiente pieza).
// Ver docs/ARQUITECTURA.md §22.
class WeaponComponent : public Node {
	GDCLASS(WeaponComponent, Node)

public:
	void _ready() override;
	void _physics_process(double p_delta) override;

	// Intenta disparar desde p_participant hacia p_direction. Devuelve true si salió una bala.
	bool try_fire(Participant *p_participant, const Vector2 &p_direction);

	// Alcance real (casillas) para este participante: puede depender de su visión (WeaponType.range_from_vision).
	float get_effective_range(const Participant *p_participant) const;

	void set_weapon(const Ref<WeaponType> &p_value);
	Ref<WeaponType> get_weapon() const { return weapon_; }
	bool is_ready() const { return state_.is_ready(); }
	int get_ammo() const { return state_.ammo(); }

protected:
	static void _bind_methods();

private:
	Ref<WeaponType> weapon_;
	lv::core::WeaponState state_;
};
