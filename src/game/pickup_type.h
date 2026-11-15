#pragma once

#include "game/stats_component.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

// Un efecto de un objeto sobre las stats: se aplica como modificador del StatsComponent.
// Ej.: stat = SPEED, op = ADD, value = 8, duration = -1 (permanente).
class PickupEffect : public Resource {
	GDCLASS(PickupEffect, Resource)

public:
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

// Tipo de objeto recolectable como DATOS (.tres): agregar un objeto nuevo = crear un archivo,
// sin tocar código. Lo usa PickupFactory para crear los Pickup.
// Ejemplos: project/data/pickups/*.tres. Ver docs/ARQUITECTURA.md §8.
class PickupType : public Resource {
	GDCLASS(PickupType, Resource)

public:
	void set_id(const String &p_value) { id_ = p_value; }
	String get_id() const { return id_; }
	void set_display_name(const String &p_value) { display_name_ = p_value; }
	String get_display_name() const { return display_name_; }
	// Título del texto flotante al recogerlo (ej. "¡RAPIDEZ!"). Vacío = solo se muestran los efectos.
	void set_popup_title(const String &p_value) { popup_title_ = p_value; }
	String get_popup_title() const { return popup_title_; }
	void set_color(const Color &p_value) { color_ = p_value; }
	Color get_color() const { return color_; }
	void set_radius(float p_value) { radius_ = p_value; }
	float get_radius() const { return radius_; }
	void set_icon(const Ref<Texture2D> &p_value) { icon_ = p_value; }
	Ref<Texture2D> get_icon() const { return icon_; }
	void set_weight(float p_value) { weight_ = p_value; }
	float get_weight() const { return weight_; }
	void set_effects(const TypedArray<PickupEffect> &p_value) { effects_ = p_value; }
	TypedArray<PickupEffect> get_effects() const { return effects_; }

protected:
	static void _bind_methods();

private:
	String id_ = "pickup";
	String display_name_ = "Objeto";
	String popup_title_;
	Color color_ = Color(1, 1, 1);
	float radius_ = 5.0f; // tamaño visual provisional (px)
	Ref<Texture2D> icon_; // arte (pieza 8); si no hay, la vista dibuja una forma de color
	float weight_ = 1.0f; // rareza: más peso = sale más seguido
	TypedArray<PickupEffect> effects_;
};
