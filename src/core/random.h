#pragma once

#include <cstdint>
#include <random>
#include <vector>

namespace lv::core {

// Generador aleatorio determinista y portable.
//
// std::mt19937 da la misma secuencia en todas las plataformas, pero las distribuciones de la
// biblioteca estándar (uniform_int_distribution, shuffle, discrete_distribution...) NO: cada
// compilador las implementa distinto. Para que una misma semilla produzca el mismo laberinto y
// los mismos objetos en Windows, Linux y en red (multijugador, repeticiones), todo el código de
// core usa esta clase en lugar de las distribuciones estándar.
class Rng {
public:
	explicit Rng(uint32_t p_seed) :
			engine_(p_seed) {}

	uint32_t next_u32() { return static_cast<uint32_t>(engine_()); }

	// Entero en [0, p_count). p_count debe ser > 0.
	int range(int p_count) {
		return static_cast<int>((static_cast<uint64_t>(next_u32()) * static_cast<uint64_t>(p_count)) >> 32);
	}

	// Real en [0, 1).
	float unit() { return static_cast<float>(next_u32() >> 8) * (1.0f / 16777216.0f); }

	bool chance(float p_probability) { return unit() < p_probability; }

	// Fisher-Yates con range(): mismo orden en todas las plataformas.
	template <typename T>
	void shuffle(std::vector<T> &r_items) {
		for (int i = static_cast<int>(r_items.size()) - 1; i > 0; i--) {
			std::swap(r_items[i], r_items[range(i + 1)]);
		}
	}

	// Índice elegido según pesos (los pesos <= 0 nunca salen). Devuelve -1 si no hay pesos positivos.
	int weighted(const std::vector<float> &p_weights) {
		float total = 0.0f;
		for (float w : p_weights) {
			total += w > 0.0f ? w : 0.0f;
		}
		if (total <= 0.0f) {
			return -1;
		}
		float pick = unit() * total;
		int last_positive = -1;
		for (int i = 0; i < static_cast<int>(p_weights.size()); i++) {
			if (p_weights[i] <= 0.0f) {
				continue;
			}
			last_positive = i;
			if (pick < p_weights[i]) {
				return i;
			}
			pick -= p_weights[i];
		}
		return last_positive; // por redondeo
	}

private:
	std::mt19937 engine_;
};

} // namespace lv::core
