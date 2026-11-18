#pragma once

#include "core/weapon.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Un arma como datos (.tres), patrón Type Object: cada arma es una combinación de piezas de datos
// (cómo dispara, cuántos perdigones, qué hace al impactar, cuánta munición), no una clase.
// Catálogo en project/data/weapons/: pistola (inicial), escopeta, láser, bazuca. Un arma nueva = un .tres.
// Envuelve a lv::core::WeaponSpec (distancias en casillas). Ver docs/ARQUITECTURA.md §22 y §23.
class WeaponType : public Resource {
	GDCLASS(WeaponType, Resource)

public:
	enum FireMode {
		FIRE_PROJECTILE = static_cast<int>(lv::core::FireMode::Projectile),
		FIRE_HELD_BEAM = static_cast<int>(lv::core::FireMode::HeldBeam),
	};

	void set_display_name(const String &p_value) { display_name_ = p_value; }
	String get_display_name() const { return display_name_; }
	void set_short_name(const String &p_value) { short_name_ = p_value; }
	String get_short_name() const { return short_name_.is_empty() ? display_name_ : short_name_; }

	void set_fire_mode(FireMode p_value) { spec_.fire_mode = static_cast<lv::core::FireMode>(p_value); }
	FireMode get_fire_mode() const { return static_cast<FireMode>(spec_.fire_mode); }
	void set_fire_interval(float p_value) { spec_.fire_interval = p_value; }
	float get_fire_interval() const { return spec_.fire_interval; }
	void set_energy(float p_value) { spec_.energy = p_value; }
	float get_energy() const { return spec_.energy; }
	void set_ammo(int p_value) { spec_.ammo = p_value; }
	int get_ammo() const { return spec_.ammo; }
	void set_magazine_size(int p_value) { spec_.magazine_size = p_value; }
	int get_magazine_size() const { return spec_.magazine_size; }
	void set_reload_time(float p_value) { spec_.reload_time = p_value; }
	float get_reload_time() const { return spec_.reload_time; }
	void set_pellets(int p_value) { spec_.pellets = p_value; }
	int get_pellets() const { return spec_.pellets; }
	void set_spread_deg(float p_value) { spec_.spread_deg = p_value; }
	float get_spread_deg() const { return spec_.spread_deg; }

	void set_projectile_speed(float p_value) { spec_.projectile_speed = p_value; }
	float get_projectile_speed() const { return spec_.projectile_speed; }
	void set_range(float p_value) { spec_.range = p_value; }
	float get_range() const { return spec_.range; }
	void set_range_from_vision(bool p_value) { spec_.range_from_vision = p_value; }
	bool get_range_from_vision() const { return spec_.range_from_vision; }
	void set_range_vision_factor(float p_value) { spec_.range_vision_factor = p_value; }
	float get_range_vision_factor() const { return spec_.range_vision_factor; }
	void set_projectile_radius(float p_value) { spec_.projectile_radius = p_value; }
	float get_projectile_radius() const { return spec_.projectile_radius; }
	void set_explosion_radius(float p_value) { spec_.explosion_radius = p_value; }
	float get_explosion_radius() const { return spec_.explosion_radius; }

	void set_damage(float p_value) { spec_.damage = p_value; }
	float get_damage() const { return spec_.damage; }
	void set_damage_edge(float p_value) { spec_.damage_edge = p_value; }
	float get_damage_edge() const { return spec_.damage_edge; }

	void set_projectile_color(const Color &p_value) { projectile_color_ = p_value; }
	Color get_projectile_color() const { return projectile_color_; }

	const lv::core::WeaponSpec &spec() const { return spec_; }

protected:
	static void _bind_methods();

private:
	String display_name_ = "Pistola";
	String short_name_;
	lv::core::WeaponSpec spec_;
	Color projectile_color_ = Color(1.0f, 0.95f, 0.6f);
};

VARIANT_ENUM_CAST(WeaponType::FireMode);
