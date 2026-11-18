#pragma once

#include "core/cell.h"
#include "core/line_of_sight.h"
#include "core/maze_grid.h"
#include "core/random.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <queue>
#include <vector>

namespace lv::core {

// Alguien que mira el laberinto (un participante): no deben aparecer objetos frente a él.
struct SpawnObserver {
	GridPoint position; // en casillas (con decimales)
	float vision_radius = 5.0f; // casillas
};

struct SpawnCellOptions {
	int min_spacing = 4; // distancia mínima a otros objetos (casillas, Chebyshev)
	int base_clearance = 6; // distancia mínima a las bases
	float observer_margin = 2.0f; // casillas extra más allá del radio de visión
	bool avoid_vault = true;
	int max_attempts = 200; // intentos aleatorios antes de rendirse
};

// Elige una casilla para que aparezca un objeto nuevo:
// suelo, fuera de la bóveda, lejos de bases y de otros objetos, y donde NINGÚN observador
// la esté viendo (fuera de su radio + margen, o tapada por paredes).
// Devuelve nullopt si no encuentra lugar (se reintenta más tarde).
std::optional<Cell> pick_spawn_cell(const MazeGrid &p_grid, const std::vector<Cell> &p_occupied,
		const std::vector<Cell> &p_bases, const std::vector<SpawnObserver> &p_observers,
		const SpawnCellOptions &p_options, Rng &r_rng);

// Igual, pero "¿hay un objeto en esta casilla?" se pregunta a un índice (p. ej. espacial): la separación
// mínima se comprueba solo en la ventana alrededor de cada candidata, no contra todos los objetos.
// Costo: O(intentos × min_spacing²) sin importar cuántos objetos haya. (RENDIMIENTO.md, regla 7)
using HasPickupAt = std::function<bool(const Cell &)>;
std::optional<Cell> pick_spawn_cell_indexed(const MazeGrid &p_grid, const HasPickupAt &p_has_pickup_at,
		const std::vector<Cell> &p_bases, const std::vector<SpawnObserver> &p_observers,
		const SpawnCellOptions &p_options, Rng &r_rng);

// Regla "mantener la densidad": si hay menos objetos que `target_count`, aparece uno nuevo
// cada `respawn_delay` segundos. Es la regla por defecto; el Director de IA (fase 5) podrá
// usar otra (patrón Strategy en la capa de Godot: PickupSpawnPolicy).
class KeepDensityRule {
public:
	int target_count = 30;
	float respawn_delay = 3.0f; // segundos entre apariciones mientras falten objetos

	// Cuántos objetos deben aparecer en este paso.
	int update(int p_active_count, double p_delta);

private:
	double timer_ = 0.0;
};

// --- Categorías de objetos (puntos, power-ups, luego armas...) -----------------------------------
// Cada categoría tiene su propio cupo, que escala con el tamaño del mapa: `p_budget` es el presupuesto
// de objetos del mapa (MazeProfile.pickup_count) y `p_share` la parte que le toca a la categoría.
// El resultado queda entre `p_min_count` y `p_max_count` (p_max_count <= 0 = sin tope).
int category_target(int p_budget, float p_share, int p_min_count, int p_max_count);

// Segundos entre apariciones para reponer el cupo completo en `p_refill_time` segundos.
// Así la reposición escala con el mapa: un mapa con el doble de cupo repone el doble de rápido.
float refill_delay(int p_target, float p_refill_time);

// Cola de caducidad: objetos que desaparecen del piso si nadie los recoge.
// Cada objeto avisa `warn_time` segundos antes (la vista parpadea) y luego caduca.
// Trabajo por frame: O(eventos que vencen), no O(objetos) — sirve con cientos de objetos.
class ExpiryQueue {
public:
	enum class Stage { Warn,
		Expire };
	struct Event {
		uint64_t id = 0;
		Stage stage = Stage::Expire;
	};

	// Programa un objeto: caduca en `p_now + p_lifetime`, avisa `p_warn_time` antes.
	// p_lifetime <= 0 = no caduca (no se programa).
	void schedule(uint64_t p_id, double p_now, double p_lifetime, double p_warn_time);
	// Eventos vencidos hasta `p_now`, en orden de tiempo. Quien llama ignora los ids que ya no existen.
	std::vector<Event> poll(double p_now);
	void clear() { queue_ = {}; }
	size_t pending() const { return queue_.size(); }

private:
	struct Entry {
		double time;
		uint64_t id;
		Stage stage;
		bool operator>(const Entry &p_other) const { return time > p_other.time; }
	};
	std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue_;
};

} // namespace lv::core
