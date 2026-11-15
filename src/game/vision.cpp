#include "game/vision.h"

#include "game/maze.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/gradient_texture2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cmath>

namespace {
// Velocidad de la transición del radio (1/s). Evita saltos bruscos al tomar un power-up.
constexpr float RADIUS_LERP_SPEED = 6.0f;
} // namespace

void Vision::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_radius_tiles"), &Vision::get_radius_tiles);
	ClassDB::bind_method(D_METHOD("set_tile_px", "value"), &Vision::set_tile_px);
	ClassDB::bind_method(D_METHOD("get_tile_px"), &Vision::get_tile_px);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tile_px", PROPERTY_HINT_RANGE, "1,128,1,suffix:px"), "set_tile_px", "get_tile_px");
}

void Vision::_ready() {
	// Textura radial generada: centro iluminado que se apaga hacia el borde.
	Ref<Gradient> gradient;
	gradient.instantiate();
	gradient->set_color(0, Color(1, 1, 1, 1));
	gradient->set_color(1, Color(1, 1, 1, 0));
	gradient->add_point(0.6f, Color(1, 1, 1, 0.9f));
	Ref<GradientTexture2D> texture;
	texture.instantiate();
	texture->set_gradient(gradient);
	texture->set_fill(GradientTexture2D::FILL_RADIAL);
	texture->set_fill_from(Vector2(0.5f, 0.5f));
	texture->set_fill_to(Vector2(0.5f, 0.0f));
	texture->set_width(TEXTURE_SIZE);
	texture->set_height(TEXTURE_SIZE);
	set_texture(texture);
	set_shadow_enabled(true);
	set_shadow_filter(SHADOW_FILTER_PCF5);
	// Ilumina todas las capas, pero todo recibe sombra salvo la capa de paredes:
	// se ven las paredes cercanas, no lo que hay detrás de ellas.
	constexpr int ALL_LAYERS = 0xFFFFF;
	set_item_cull_mask(ALL_LAYERS);
	set_item_shadow_cull_mask(ALL_LAYERS & ~Maze::WALLS_LIGHT_MASK);

	if (Engine::get_singleton()->is_editor_hint()) {
		apply_radius();
		return;
	}

	// Buscar el StatsComponent hermano (hijo del mismo participante).
	StatsComponent *stats = nullptr;
	if (Node *parent = get_parent()) {
		for (int i = 0; i < parent->get_child_count() && stats == nullptr; i++) {
			stats = Object::cast_to<StatsComponent>(parent->get_child(i));
		}
	}
	if (stats == nullptr) {
		UtilityFunctions::push_warning("Vision: no hay StatsComponent hermano; se usa un radio fijo.");
	} else {
		stats->connect("stat_changed", callable_mp(this, &Vision::on_stat_changed));
		target_radius_ = stats->get_vision();
	}
	current_radius_ = target_radius_;
	apply_radius();
}

void Vision::_process(double p_delta) {
	if (std::abs(current_radius_ - target_radius_) < 0.01f) {
		return;
	}
	const float t = 1.0f - std::exp(-RADIUS_LERP_SPEED * static_cast<float>(p_delta));
	current_radius_ += (target_radius_ - current_radius_) * t;
	apply_radius();
}

void Vision::on_stat_changed(int p_stat, float p_value) {
	if (p_stat == StatsComponent::STAT_VISION) {
		target_radius_ = p_value;
	}
}

void Vision::apply_radius() {
	// La textura mide TEXTURE_SIZE px de diámetro; se escala para que el radio sea el pedido.
	const float radius_px = current_radius_ * tile_px_;
	set_texture_scale(radius_px * 2.0f / TEXTURE_SIZE);
}
