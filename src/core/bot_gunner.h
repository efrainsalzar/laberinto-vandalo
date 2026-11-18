#pragma once

#include "core/random.h"

#include <vector>

namespace lv::core {

// Un rival que el bot VE (quien llama ya filtró por radio y línea de visión: juego limpio). En casillas.
struct GunTarget {
	int id = -1;
	float x = 0.0f;
	float y = 0.0f;
};

struct GunDecision {
	bool fire = false;
	float aim_x = 0.0f; // dirección (sin normalizar); (0, 0) = sin blanco
	float aim_y = 0.0f;
	int target = -1;
};

// Gatillo de un bot. Va APARTE de BotBrain: BotBrain elige a dónde ir (una BotAction = un destino) y el bot
// tiene que poder moverse y disparar a la vez. Lo combina BotController en el mismo ParticipantCommand.
//
// - Elige al rival visible más cercano.
// - Tiempo de reacción: al ver a un rival nuevo espera `reaction_time` antes del primer disparo.
// - Puntería imperfecta: cada disparo se desvía un ángulo al azar de ±`aim_error_deg` (dificultad).
// - Solo dispara dentro de `max_range` casillas (no gasta balas en blancos que no alcanza).
// La cadencia y la munición las controla el arma (WeaponState), no el bot. Ver docs/ARQUITECTURA.md §22.
class BotGunner {
public:
	struct Tuning {
		float reaction_time = 0.5f;
		float aim_error_deg = 12.0f;
		float max_range = 10.0f;
	};

	void setup(const Tuning &p_tuning);
	// p_targets: rivales visibles ahora. Devuelve si dispara y hacia dónde.
	GunDecision update(float p_delta, float p_self_x, float p_self_y, const std::vector<GunTarget> &p_targets, Rng &r_rng);
	// El alcance depende del arma que lleve (puede cambiar en la partida).
	void set_max_range(float p_range) { tuning_.max_range = p_range; }
	// Olvida el blanco (p. ej. al reaparecer).
	void reset();

	int current_target() const { return target_; }
	const Tuning &tuning() const { return tuning_; }

private:
	Tuning tuning_;
	int target_ = -1;
	float wait_ = 0.0f;
};

} // namespace lv::core
