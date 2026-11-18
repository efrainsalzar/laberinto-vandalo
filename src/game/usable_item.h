#pragma once

#include "core/explosives.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Un objeto que se guarda y se usa después (bomba, mina) como DATOS (.tres), patrón Type Object como WeaponType.
// Catálogo en project/data/items/. Envuelve a lv::core::ExplosiveSpec (casillas, segundos) + daño y aspecto.
// Ver docs/ARQUITECTURA.md §28.
class UsableItem : public Resource {
	GDCLASS(UsableItem, Resource)

public:
	enum Kind {
		KIND_BOMB = static_cast<int>(lv::core::ExplosiveKind::Bomb),
		KIND_MINE = static_cast<int>(lv::core::ExplosiveKind::Mine),
	};

	void set_display_name(const String &p_value) { display_name_ = p_value; }
	String get_display_name() const { return display_name_; }
	void set_short_name(const String &p_value) { short_name_ = p_value; }
	String get_short_name() const { return short_name_.is_empty() ? display_name_ : short_name_; }
	void set_kind(Kind p_value) { spec_.kind = static_cast<lv::core::ExplosiveKind>(p_value); }
	Kind get_kind() const { return static_cast<Kind>(spec_.kind); }
	void set_max_carry(int p_value) { max_carry_ = p_value < 1 ? 1 : p_value; }
	int get_max_carry() const { return max_carry_; }

	void set_fuse(float p_value) { spec_.fuse = p_value; }
	float get_fuse() const { return spec_.fuse; }
	void set_arm_time(float p_value) { spec_.arm_time = p_value; }
	float get_arm_time() const { return spec_.arm_time; }
	void set_trigger_radius(float p_value) { spec_.trigger_radius = p_value; }
	float get_trigger_radius() const { return spec_.trigger_radius; }
	void set_explosion_radius(float p_value) { spec_.explosion_radius = p_value; }
	float get_explosion_radius() const { return spec_.explosion_radius; }
	void set_damage(float p_value) { damage_ = p_value; }
	float get_damage() const { return damage_; }
	void set_damage_edge(float p_value) { damage_edge_ = p_value; }
	float get_damage_edge() const { return damage_edge_; }
	void set_color(const Color &p_value) { color_ = p_value; }
	Color get_color() const { return color_; }

	const lv::core::ExplosiveSpec &spec() const { return spec_; }

protected:
	static void _bind_methods();

private:
	String display_name_ = "Bomba";
	String short_name_;
	int max_carry_ = 3;
	lv::core::ExplosiveSpec spec_;
	float damage_ = 80.0f; // en el centro de la explosión
	float damage_edge_ = 30.0f; // en el borde
	Color color_ = Color(1.0f, 0.55f, 0.2f);
};

VARIANT_ENUM_CAST(UsableItem::Kind);
