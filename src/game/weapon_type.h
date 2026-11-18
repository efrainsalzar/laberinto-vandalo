#pragma once

#include "core/weapon.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Un arma como datos (.tres), patrón Type Object: la pistola inicial es project/data/weapons/pistol.tres y las
// armas temporales del mapa serán más .tres, sin código nuevo. Envuelve a lv::core::WeaponSpec (distancias en
// casillas, así un arma funciona igual con pasillos anchos o angostos). Ver docs/ARQUITECTURA.md §22.
class WeaponType : public Resource {
	GDCLASS(WeaponType, Resource)

public:
	void set_display_name(const String &p_value) { display_name_ = p_value; }
	String get_display_name() const { return display_name_; }
	void set_fire_interval(float p_value) { spec_.fire_interval = p_value; }
	float get_fire_interval() const { return spec_.fire_interval; }
	void set_magazine_size(int p_value) { spec_.magazine_size = p_value; }
	int get_magazine_size() const { return spec_.magazine_size; }
	void set_reload_time(float p_value) { spec_.reload_time = p_value; }
	float get_reload_time() const { return spec_.reload_time; }
	void set_projectile_speed(float p_value) { spec_.projectile_speed = p_value; }
	float get_projectile_speed() const { return spec_.projectile_speed; }
	void set_range(float p_value) { spec_.range = p_value; }
	float get_range() const { return spec_.range; }
	void set_projectile_radius(float p_value) { spec_.projectile_radius = p_value; }
	float get_projectile_radius() const { return spec_.projectile_radius; }
	void set_projectile_color(const Color &p_value) { projectile_color_ = p_value; }
	Color get_projectile_color() const { return projectile_color_; }

	const lv::core::WeaponSpec &spec() const { return spec_; }

protected:
	static void _bind_methods();

private:
	String display_name_ = "Pistola";
	lv::core::WeaponSpec spec_;
	Color projectile_color_ = Color(1.0f, 0.95f, 0.6f);
};
