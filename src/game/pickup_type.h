#pragma once

#include "game/pickup_effect.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

// Tipo de objeto recolectable como DATOS (.tres): agregar un objeto nuevo = crear un archivo,
// sin tocar código. Lo usa PickupFactory para crear los Pickup. Lo que hace al recogerlo = su lista de efectos.
// Ejemplos: project/data/pickups/*.tres. Ver docs/ARQUITECTURA.md §8.
class PickupType : public Resource {
	GDCLASS(PickupType, Resource)

public:
	// Lenguaje visual (§21): la FORMA dice la categoría y el SÍMBOLO dice qué hace.
	enum Shape {
		SHAPE_CIRCLE, // mejora de stat
		SHAPE_DIAMOND, // power-up temporal
		SHAPE_SQUARE, // arma
		SHAPE_HEXAGON, // objeto que cambia el mapa
		SHAPE_RING, // portal
		SHAPE_FLASK, // poción (curación, §26)
		SHAPE_TRIANGLE, // objeto que se guarda y se usa después (bomba, mina; §28)
	};
	enum Symbol {
		SYMBOL_NONE,
		SYMBOL_SPEED, // flechas »
		SYMBOL_VISION, // ojo
		SYMBOL_BOLT, // rayo
		SYMBOL_SPREAD, // abanico de 3 líneas (escopeta)
		SYMBOL_BEAM, // línea con destello (láser)
		SYMBOL_BLAST, // estrella de explosión (bazuca)
		SYMBOL_CROSS, // cruz + (curar)
		SYMBOL_BUBBLE, // círculo (escudo burbuja)
		SYMBOL_ARC, // arco (escudo frontal)
		SYMBOL_MIRROR, // flecha que rebota (escudo espejo)
		SYMBOL_BOMB, // bomba con mecha
		SYMBOL_MINE, // mina: disco con púas
	};
	void set_shape(Shape p_value) { shape_ = p_value; }
	Shape get_shape() const { return shape_; }
	void set_symbol(Symbol p_value) { symbol_ = p_value; }
	Symbol get_symbol() const { return symbol_; }

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
	// ¿Lo puede recoger ahora? Sí si algún efecto le sirve (una poción con la salud llena se queda en el piso).
	bool is_useful_for(Participant *p_participant) const;
	// Aplica todos sus efectos (Strategy: cada efecto sabe aplicarse). §26.
	void apply_to(Participant *p_participant) const;

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
	Shape shape_ = SHAPE_CIRCLE;
	Symbol symbol_ = SYMBOL_NONE;
};

VARIANT_ENUM_CAST(PickupType::Shape);
VARIANT_ENUM_CAST(PickupType::Symbol);
