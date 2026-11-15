#pragma once

#include "core/maze_shift.h"
#include "core/random.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <memory>
#include <vector>

using namespace godot;

class Maze;

// Laberinto vivo: cada cierto tiempo, abre pasajes y cierra otros (el laberinto cambia de forma).
//
// Máquina de estados (patrón State):
//   IDLE ──intervalo──► WARNING (aviso: las casillas parpadean) ──► APPLYING ──► IDLE
// - Aperturas: se aplican al terminar el aviso (siempre seguras).
// - Cierres: solo si NADIE está encima (casilla + margen). Si sigue ocupado `max_block_wait` segundos,
//   ese cierre se CANCELA: nunca se aplasta ni se encierra a nadie. Cancelar un cierre es seguro
//   (lv::core::plan_maze_shift garantiza conectividad con cualquier subconjunto de cierres).
// - Red de seguridad: tras un cierre, si un participante quedara dentro de una pared, se lo mueve
//   al suelo libre más cercano.
// La lógica (qué cambiar sin desconectar) está en core; aquí se gestiona tiempo, avisos y ocupación.
// Ver docs/ARQUITECTURA.md §15.
class MazeShifter : public Node {
	GDCLASS(MazeShifter, Node)

public:
	enum State {
		STATE_IDLE,
		STATE_WARNING,
		STATE_APPLYING,
	};

	void _process(double p_delta) override;

	void start(Maze *p_maze, int p_seed);
	void stop();
	// Empieza un cambio ya (sin esperar el intervalo). Útil para pruebas.
	void shift_now();
	State get_state() const { return state_; }

	void set_enabled(bool p_value) { enabled_ = p_value; }
	bool is_enabled() const { return enabled_; }
	void set_interval(float p_value) { interval_ = p_value; }
	float get_interval() const { return interval_; }
	void set_pairs(int p_value) { pairs_ = p_value; }
	int get_pairs() const { return pairs_; }
	void set_warning_time(float p_value) { warning_time_ = p_value; }
	float get_warning_time() const { return warning_time_; }
	void set_max_block_wait(float p_value) { max_block_wait_ = p_value; }
	float get_max_block_wait() const { return max_block_wait_; }
	void set_block_margin(float p_value) { block_margin_ = p_value; }
	float get_block_margin() const { return block_margin_; }

protected:
	static void _bind_methods();

private:
	struct PendingClosure {
		lv::core::PassageChange change;
		float waited = 0.0f;
	};

	Maze *maze_ = nullptr;
	std::unique_ptr<lv::core::Rng> rng_;
	State state_ = STATE_IDLE;
	float timer_ = 0.0f;
	std::vector<lv::core::PassageChange> planned_;
	std::vector<PendingClosure> closures_;

	bool enabled_ = true;
	float interval_ = 15.0f;
	int pairs_ = 2;
	float warning_time_ = 2.5f;
	float max_block_wait_ = 3.0f;
	float block_margin_ = 14.0f; // px alrededor de cada casilla que se cierra

	void begin_warning();
	void begin_applying();
	void update_closures(double p_delta);
	bool is_occupied(const lv::core::PassageChange &p_change) const;
	void rescue_trapped_participants();
	TypedArray<Vector2i> tiles_of(const lv::core::PassageChange &p_change) const;
};

VARIANT_ENUM_CAST(MazeShifter::State);
