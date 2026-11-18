#pragma once

#include <array>
#include <string>
#include <vector>

namespace lv::core {

// Stats que puede tener un participante. Se agregarán más (resistencia, ataque) en la fase de combate.
enum class StatType : int {
	Speed = 0, // píxeles por segundo
	Vision = 1, // radio de visión en casillas
	Count
};

enum class ModifierOp : int {
	Add = 0, // suma al valor base
	Multiply = 1, // multiplica el resultado
};

// Un efecto sobre una stat: "+10 de velocidad (permanente, por un punto)",
// "×1.5 de velocidad durante 5 s (power-up)", "×0.6 mientras lleva el trofeo".
struct StatModifier {
	StatType stat = StatType::Speed;
	ModifierOp op = ModifierOp::Add;
	float value = 0.0f;
	double duration = PERMANENT; // segundos; PERMANENT = no vence
	std::string source; // de dónde viene ("point", "trophy"...): permite quitar todos los de un origen

	static constexpr double PERMANENT = -1.0;
};

// Valores base + pila de modificadores (variante de Decorator usada en juegos).
//
//   valor = clamp((base + Σ sumas) × Π multiplicadores, mínimo, máximo)
//
// Lógica pura: no sabe de Godot ni de participantes. Ver docs/ARQUITECTURA.md §6.
class Stats {
public:
	Stats();

	void set_base(StatType p_stat, float p_value);
	float get_base(StatType p_stat) const;
	void set_limits(StatType p_stat, float p_min, float p_max);

	// Valor final con todos los modificadores aplicados.
	float get(StatType p_stat) const;

	// Tope de lo que suman las mejoras PERMANENTES positivas (Add) de una stat: p. ej. los puntos
	// recolectables no pueden dar más de +48 de velocidad en total. < 0 = sin tope.
	// No afecta a los efectos temporales ni a los multiplicadores (power-ups, trofeo).
	void set_bonus_cap(StatType p_stat, float p_cap);
	// Lo que suman hoy las mejoras permanentes positivas de una stat.
	float permanent_bonus(StatType p_stat) const;
	// ¿Las mejoras permanentes de esta stat ya llegaron a su tope?
	bool is_bonus_maxed(StatType p_stat) const;

	// Devuelve un id para poder quitarlo después, o 0 si no se agregó (tope de mejora alcanzado).
	// Si la mejora pasa el tope, se recorta a lo que falta.
	int add_modifier(const StatModifier &p_modifier);
	bool remove_modifier(int p_id);
	// Quita todos los modificadores de un origen. Devuelve cuántos quitó.
	int remove_modifiers_from(const std::string &p_source);

	// Avanza el tiempo y elimina los modificadores vencidos.
	// Devuelve las stats que cambiaron por eso (sin repetir).
	std::vector<StatType> tick(double p_delta);

	struct ActiveModifier {
		int id;
		StatModifier modifier;
		double remaining; // segundos restantes; PERMANENT si no vence
	};
	const std::vector<ActiveModifier> &modifiers() const { return modifiers_; }

private:
	static constexpr size_t STAT_COUNT = static_cast<size_t>(StatType::Count);

	std::array<float, STAT_COUNT> base_{};
	std::array<float, STAT_COUNT> min_{};
	std::array<float, STAT_COUNT> max_{};
	std::array<float, STAT_COUNT> bonus_cap_{};
	std::vector<ActiveModifier> modifiers_;
	int next_id_ = 1;
};

} // namespace lv::core
