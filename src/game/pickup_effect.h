#pragma once

#include "game/stats_component.h"
#include "game/usable_item.h"
#include "game/weapon_type.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class Participant;

// Lo que hace un objeto al recogerlo (patrón Strategy + datos): cada efecto es un Resource que sabe aplicarse
// al participante. Un PickupType tiene una lista de efectos (.tres); un efecto nuevo = una subclase nueva, sin
// tocar Pickup ni los demás efectos.
//   StatEffect   → modificador de stat (mejoras permanentes y power-ups)
//   HealEffect   → cura (pociones)
//   WeaponEffect → da un arma especial (escopeta, láser, bazuca)
//   ShieldEffect → da un escudo (burbuja, frontal, espejo; §27)
//   GiveItemEffect → guarda un objeto usable en la mochila (bomba, mina; §28)
// Ver docs/ARQUITECTURA.md §26.
class PickupEffect : public Resource {
	GDCLASS(PickupEffect, Resource)

public:
	// ¿Sirve de algo ahora? Si ningún efecto del objeto sirve, el objeto no se recoge (p. ej. una poción con la
	// salud llena se queda en el piso para otro).
	virtual bool is_useful(Participant *p_participant) const { return true; }
	// Aplica el efecto. `p_source` identifica el objeto (para los modificadores de stats).
	virtual void apply(Participant *p_participant, const String &p_source) const {}

protected:
	static void _bind_methods() {}
};

// Modificador de stat. Ej.: stat = SPEED, op = ADD, value = 8, duration = -1 (permanente).
class StatEffect : public PickupEffect {
	GDCLASS(StatEffect, PickupEffect)

public:
	void apply(Participant *p_participant, const String &p_source) const override;

	void set_stat(StatsComponent::Stat p_value) { stat_ = p_value; }
	StatsComponent::Stat get_stat() const { return stat_; }
	void set_op(StatsComponent::Op p_value) { op_ = p_value; }
	StatsComponent::Op get_op() const { return op_; }
	void set_value(float p_value) { value_ = p_value; }
	float get_value() const { return value_; }
	void set_duration(float p_value) { duration_ = p_value; }
	float get_duration() const { return duration_; }

protected:
	static void _bind_methods();

private:
	StatsComponent::Stat stat_ = StatsComponent::STAT_SPEED;
	StatsComponent::Op op_ = StatsComponent::OP_ADD;
	float value_ = 0.0f;
	float duration_ = -1.0f; // < 0 = permanente
};

// Cura `amount` de salud (sin pasar del máximo). Solo sirve si al participante le falta salud. §25.
class HealEffect : public PickupEffect {
	GDCLASS(HealEffect, PickupEffect)

public:
	bool is_useful(Participant *p_participant) const override;
	void apply(Participant *p_participant, const String &p_source) const override;

	void set_amount(float p_value) { amount_ = p_value; }
	float get_amount() const { return amount_; }

protected:
	static void _bind_methods();

private:
	float amount_ = 25.0f;
};

// Da un arma: va a la ranura especial del WeaponComponent y se equipa sola (reemplaza a la anterior). §23.
class WeaponEffect : public PickupEffect {
	GDCLASS(WeaponEffect, PickupEffect)

public:
	void apply(Participant *p_participant, const String &p_source) const override;

	void set_weapon(const Ref<WeaponType> &p_value) { weapon_ = p_value; }
	Ref<WeaponType> get_weapon() const { return weapon_; }

protected:
	static void _bind_methods();

private:
	Ref<WeaponType> weapon_;
};

// Da un escudo (ShieldComponent::Kind). duration <= 0 = la duración por defecto de ese escudo. §27.
class ShieldEffect : public PickupEffect {
	GDCLASS(ShieldEffect, PickupEffect)

public:
	void apply(Participant *p_participant, const String &p_source) const override;

	void set_kind(int p_value) { kind_ = p_value; }
	int get_kind() const { return kind_; }
	void set_duration(float p_value) { duration_ = p_value; }
	float get_duration() const { return duration_; }

protected:
	static void _bind_methods();

private:
	int kind_ = 0;
	float duration_ = 0.0f;
};

// Guarda un objeto usable (bomba, mina) en la mochila del participante (ItemComponent). No se usa al recogerlo.
// Si ya lleva el máximo de ese objeto, no sirve: queda en el piso. §28.
class GiveItemEffect : public PickupEffect {
	GDCLASS(GiveItemEffect, PickupEffect)

public:
	bool is_useful(Participant *p_participant) const override;
	void apply(Participant *p_participant, const String &p_source) const override;

	void set_item(const Ref<UsableItem> &p_value) { item_ = p_value; }
	Ref<UsableItem> get_item() const { return item_; }
	void set_count(int p_value) { count_ = p_value < 1 ? 1 : p_value; }
	int get_count() const { return count_; }

protected:
	static void _bind_methods();

private:
	Ref<UsableItem> item_;
	int count_ = 1;
};
