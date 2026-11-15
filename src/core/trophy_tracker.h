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
		};
		Type type;
		int participant = -1;
		Cell cell;
	};

	static constexpr int NO_ONE = -1;

	// p_bases[i] es la base del participante i. p_pickup_range y p_base_radius en casillas (Chebyshev).
	void setup(const Cell &p_trophy_cell, const std::vector<Cell> &p_bases, int p_pickup_range = 1, int p_base_radius = 1);

	// Informa dónde está un participante; devuelve los eventos que eso produjo.
	std::vector<Event> update(int p_participant, const Cell &p_cell);
	// El portador suelta el trofeo en p_cell (muerte en la fase de combate, o a propósito).
	std::vector<Event> drop(const Cell &p_cell);

	State state() const { return state_; }
	int carrier() const { return carrier_; }
	int winner() const { return winner_; }
	const Cell &trophy_cell() const { return trophy_cell_; } // válido en InVault / Dropped
	const Cell &base_of(int p_participant) const { return bases_[p_participant]; }
	int participant_count() const { return static_cast<int>(bases_.size()); }
	bool is_in_own_base(int p_participant, const Cell &p_cell) const;

private:
	State state_ = State::InVault;
	Cell trophy_cell_;
	std::vector<Cell> bases_;
	int carrier_ = NO_ONE;
	int winner_ = NO_ONE;
	int pickup_range_ = 1;
	int base_radius_ = 1;

	bool valid(int p_participant) const { return p_participant >= 0 && p_participant < participant_count(); }
};

} // namespace lv::core
