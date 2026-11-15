#pragma once

#include "game/stats_component.h"

#include <godot_cpp/classes/point_light2d.hpp>

using namespace godot;

// Visión limitada de un participante: una luz que solo ilumina a su alrededor.
// Las paredes del Maze proyectan sombra, así que no se ve a través de ellas.
//
// - Composición: es un nodo hijo más del Participant (como Stats, Collision o View).
// - Observer: escucha `stat_changed` del StatsComponent hermano; si la visión cambia
//   (power-up), el radio se ajusta solo.
// - Solo tiene sentido para quien mira la pantalla: los bots usan Maze::can_see (lógica pura).
// Para que haya oscuridad, el nivel necesita un CanvasModulate oscuro.
// Ver docs/ARQUITECTURA.md §7.
class Vision : public PointLight2D {
	GDCLASS(Vision, PointLight2D)

public:
	void _ready() override;
	void _process(double p_delta) override;

	// Radio actual en casillas (sigue a la stat de visión con una transición suave).
	float get_radius_tiles() const { return current_radius_; }

	void set_tile_px(float p_value) { tile_px_ = p_value; }
	float get_tile_px() const { return tile_px_; }

protected:
	static void _bind_methods();

private:
	static constexpr int TEXTURE_SIZE = 256;

	float tile_px_ = 16.0f;
	float target_radius_ = 5.0f; // casillas
	float current_radius_ = 5.0f;

	void on_stat_changed(int p_stat, float p_value);
	void apply_radius();
};
