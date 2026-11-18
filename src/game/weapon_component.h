#pragma once

#include "core/weapon.h"
#include "game/fire_behavior.h"
#include "game/weapon_type.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <memory>

using namespace godot;

class Participant;

// Las armas que lleva un participante, como componente hijo (composición, igual que StatsComponent y
// LifeComponent).
//
// Inventario (decisión del dueño): **pistola fija + 1 ranura especial**.
// - `weapon` es el arma base (la pistola, munición infinita).
// - equip_special(): al recoger un arma del mapa se equipa sola (reemplaza a la especial anterior).
// - Cuando la especial se queda sin munición o batería, o el participante es eliminado, se vuelve a la pistola.
//
// Cada ranura tiene su lv::core::WeaponState (cadencia, munición, batería) y su FireBehavior (Strategy: CÓMO
// dispara). Participant::execute_command llama a update_trigger() cada frame con el gatillo abajo o arriba; aquí se
// calcula si se apretó, se mantiene o se soltó, y el comportamiento del arma activa decide qué hacer.
// Señales para la vista y el HUD: fired(direction), weapon_changed(weapon) (el arma activa),
// charge_changed(fraction) (munición o batería de la especial, 0..1). Ver docs/ARQUITECTURA.md §22 y §23.
class WeaponComponent : public Node {
	GDCLASS(WeaponComponent, Node)

public:
	void _ready() override;
	void _physics_process(double p_delta) override;

	// El gatillo de este frame (abajo o arriba) y hacia dónde apunta. Devuelve true si disparó.
	bool update_trigger(Participant *p_participant, bool p_down, const Vector2 &p_aim, float p_delta);

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
	// Disparos que le quedan a la especial (-1 = infinitos o el arma usa batería; 0 = no hay especial).
	int get_special_ammo() const;
	// Batería de la especial en segundos (-1 = no usa batería).
	float get_special_energy() const;
	// Lo que le queda a la especial, de 0 a 1 (munición o batería). 1 si es infinita; 0 si no hay especial.
	float get_special_charge() const;

	// La que dispara ahora: la especial si hay, si no la base.
	Ref<WeaponType> get_active_weapon() const { return special_.is_valid() ? special_ : weapon_; }
	bool is_ready() const { return active_state().is_ready(); }

protected:
	static void _bind_methods();

private:
	Ref<WeaponType> weapon_;
	lv::core::WeaponState base_state_;
	std::unique_ptr<FireBehavior> base_behavior_;
	Ref<WeaponType> special_;
	lv::core::WeaponState special_state_;
	std::unique_ptr<FireBehavior> special_behavior_;
	bool trigger_down_ = false;

	const lv::core::WeaponState &active_state() const { return special_.is_valid() ? special_state_ : base_state_; }
	void on_owner_eliminated(Object *p_by);
};
