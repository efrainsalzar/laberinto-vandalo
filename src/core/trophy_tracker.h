#pragma once

#include "core/cell.h"

#include <vector>

namespace lv::core {

// Reglas del trofeo y las bases (VISION §3, §6, §16). Lógica pura: recibe "el participante X está en la
// casilla Y" y devuelve lo que pasó. No sabe dónde está la bóveda en el mundo (solo una casilla), así
// que funciona igual cuando la bóveda sea un mini-mapa propio (DISENO_MAPA §3).
//
// Estados del trofeo (patrón State, como enum: son pocos y sin comportamiento propio):
//   InVault ──tomar──► Carried ──soltar──► Dropped ──tomar──► Carried ──llegar a SU base──► Delivered
//                       Carried ──otro lo toca (steal)──► Carried (nuevo portador, inmune un rato)
//
// Robo básico: quien toca al portador le quita el trofeo. Para que no se lo pasen de mano en mano cada frame,
// el nuevo portador queda INMUNE `steal_immunity` segundos (el reloj avanza con tick()). Cómo se detecta el
// "toque" lo decide quien llama (TrophyController por distancia; luego, un golpe en el combate).
//
// Ver docs/ARQUITECTURA.md §11.
class TrophyTracker {
public:
	enum class State {
		InVault, // en su pedestal de la bóveda
		Carried, // lo lleva un participante
		Dropped, // en el piso (su portador murió o lo soltó)
		Delivered, // llegó a una base: hay ganador
	};

	struct Event {
		enum class Type {
			Taken, // participant tomó el trofeo
			Dropped, // el trofeo cayó en `cell`
			Delivered, // participant lo llevó a su base: gana
			Stolen, // participant le quitó el trofeo a `victim`
		};
		Type type;
		int participant = -1;
		Cell cell;
		int victim = -1; // solo en Stolen
	};

	static constexpr int NO_ONE = -1;

	// p_zones[i] es la zona de resguardo del participante i (rectángulo de casillas: mide lo que mide el
	// pasillo, ver MazeGrid::base_zones). p_pickup_range en casillas (Chebyshev).
	void setup(const Cell &p_trophy_cell, const std::vector<TileRect> &p_zones, int p_pickup_range = 1);
	// Atajo: bases como casilla + radio (cuadro de (2r+1)x(2r+1)).
	void setup(const Cell &p_trophy_cell, const std::vector<Cell> &p_bases, int p_pickup_range, int p_base_radius);

	// Informa dónde está un participante; devuelve los eventos que eso produjo.
	std::vector<Event> update(int p_participant, const Cell &p_cell);
	// El portador suelta el trofeo en p_cell (muerte en la fase de combate, o a propósito).
	std::vector<Event> drop(const Cell &p_cell);

	// --- Robo ---
	void set_steal_immunity(float p_seconds) { steal_immunity_ = p_seconds < 0.0f ? 0.0f : p_seconds; }
	float steal_immunity() const { return steal_immunity_; }
	// Avanza el reloj de la inmunidad.
	void tick(float p_delta);
	// ¿p_thief puede quitarle el trofeo al portador ahora? (lo lleva otro y ya pasó la inmunidad)
	bool can_steal(int p_thief) const;
	// p_thief le quita el trofeo al portador (en p_cell). Vacío si no se puede.
	std::vector<Event> steal(int p_thief, const Cell &p_cell);
	float immunity_left() const { return immunity_left_; }
	// Tras caer al piso, nadie puede tomarlo durante estos segundos (que no lo agarre siempre quien estaba encima).
	void set_drop_lock(float p_seconds) { drop_lock_ = p_seconds < 0.0f ? 0.0f : p_seconds; }
	float drop_lock_left() const { return drop_lock_left_; }

	// Mueve el trofeo que está en el piso o en la bóveda (p. ej. una pared se cerró encima).
	// No hace nada si alguien lo lleva o ya fue entregado. Devuelve true si lo movió.
	bool relocate(const Cell &p_cell);

	State state() const { return state_; }
	int carrier() const { return carrier_; }
	int winner() const { return winner_; }
	const Cell &trophy_cell() const { return trophy_cell_; } // válido en InVault / Dropped
	const TileRect &base_of(int p_participant) const { return zones_[p_participant]; }
	int participant_count() const { return static_cast<int>(zones_.size()); }
	bool is_in_own_base(int p_participant, const Cell &p_cell) const;

private:
	State state_ = State::InVault;
	Cell trophy_cell_;
	std::vector<TileRect> zones_;
	int carrier_ = NO_ONE;
	int winner_ = NO_ONE;
	int pickup_range_ = 1;
	float steal_immunity_ = 3.0f;
	float immunity_left_ = 0.0f;
	float drop_lock_ = 0.0f;
	float drop_lock_left_ = 0.0f;

	bool valid(int p_participant) const { return p_participant >= 0 && p_participant < participant_count(); }
};

} // namespace lv::core
