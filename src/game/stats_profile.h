#pragma once

#include <godot_cpp/classes/resource.hpp>

using namespace godot;

// Valores base de las stats de un tipo de participante, como archivo de datos (.tres).
// Cada personaje (o bot con personalidad) tendrá su propio perfil sin tocar código.
// Ejemplo: project/data/stats/default_participant.tres
class StatsProfile : public Resource {
	GDCLASS(StatsProfile, Resource)

public:
	void set_speed(float p_value) { speed_ = p_value; }
	float get_speed() const { return speed_; }
	void set_vision(float p_value) { vision_ = p_value; }
	float get_vision() const { return vision_; }
	// Máximo que pueden sumar en total las mejoras permanentes (puntos recolectables). < 0 = sin tope.
	void set_speed_bonus_max(float p_value) { speed_bonus_max_ = p_value; }
	float get_speed_bonus_max() const { return speed_bonus_max_; }
	void set_vision_bonus_max(float p_value) { vision_bonus_max_ = p_value; }
	float get_vision_bonus_max() const { return vision_bonus_max_; }

protected:
	static void _bind_methods();

private:
	float speed_ = 160.0f; // px/s
	float vision_ = 5.0f; // casillas
	float speed_bonus_max_ = 48.0f; // +40 % sobre 120
	float vision_bonus_max_ = 2.5f; // +50 % sobre 5
};
