#pragma once

#include "core/weapon.h"
#include "game/weapon_type.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

using namespace godot;

class Participant;

// Las armas que lleva un participante, como componente hijo (composición, igual que StatsComponent y
// LifeComponent). Cada arma en uso es un lv::core::WeaponState (State: lista / enfriando / recargando).
//
// Inventario (decisión del dueño): **pistola fija + 1 ranura especial**.
// - `weapon` es el arma base (la pistola, munición infinita).
// - equip_special(): al recoger un arma del mapa se equipa sola (reemplaza a la especial anterior).
// - Cuando la especial se queda sin munición, o el participante es eliminado, se vuelve a la pistola.
// - Participant::execute_command le pasa la orden de disparo; el arma activa pide balas o rayos a
//   ProjectileSystem (grupo "projectile_system").
// Señales para la vista y el HUD: fired(direction), weapon_changed(weapon) (el arma activa), ammo_changed(ammo).
// Ver docs/ARQUITECTURA.md §22 y §23.
class WeaponComponent : public Node {
	GDCLASS(WeaponComponent, Node)

public:
	void _ready() override;
	void _physics_process(double p_delta) override;

	// Intenta disparar el arma activa desde p_participant hacia p_direction. Devuelve true si disparó.
	bool try_fire(Participant *p_participant, const Vector2 &p_direction);

	// Alcance real (casillas) del arma activa para este participante (puede depender de su visión).
	float get_effective_range(const Participant *p_participant) const;

	// Arma base (pistola).
	void set_weapon(const Ref<WeaponType> &p_value);
	Ref<WeaponType> get_weapon() const { return weapon_; }

	// Ranura especial.
	void equip_special(const Ref<WeaponType> &p_value);
	void clear_special();
	Ref<WeaponType> get_special() const { return special_; }
	bool has_special() const { return special_.is_valid(); }
	// Disparos que le quedan a la especial (-1 = infinitos; 0 = no hay especial).
	int get_special_ammo() const;

	// La que dispara ahora: la especial si hay, si no la base.
	Ref<WeaponType> get_active_weapon() const { return special_.is_valid() ? special_ : weapon_; }
	bool is_ready() const { return active_state().is_ready(); }

protected:
	static void _bind_methods();

private:
	Ref<WeaponType> weapon_;
	lv::core::WeaponState base_state_;
	Ref<WeaponType> special_;
	lv::core::WeaponState special_state_;
	std::vector<lv::core::Direction> directions_; // reutilizado al disparar perdigones

	const lv::core::WeaponState &active_state() const { return special_.is_valid() ? special_state_ : base_state_; }
	lv::core::WeaponState &active_state() { return special_.is_valid() ? special_state_ : base_state_; }
	void on_owner_eliminated(Object *p_by);
};
