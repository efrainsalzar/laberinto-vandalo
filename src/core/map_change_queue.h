#pragma once

#include "core/maze_shift.h"

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

namespace lv::core {

// Prioridad de quien pide un cambio del mapa (mayor gana). Ver docs/RENDIMIENTO.md (regla 4).
enum class ChangePriority : int {
	Ambient = 0, // laberinto vivo automático
	Director = 1, // Director de IA
	Player = 2, // acciones de jugadores y power-ups
	Safety = 3, // rescates y correcciones de seguridad
};

// Orden de cambio del mapa (patrón Command): qué pasaje, abrir/cerrar, quién y con qué prioridad.
struct MapChangeOrder {
	PassageChange change;
	ChangePriority priority = ChangePriority::Ambient;
	int source = 0; // identificador de la fuente (para reservas)
};

// Cola de órdenes de cambio del mapa con prioridades, reservas, tope y presupuesto por frame.
//
// - Reservas: una fuente reserva un pasaje mientras su cambio está anunciado (aviso). Otra fuente solo
//   puede tocarlo con prioridad MAYOR; en ese caso la reserva anterior se pierde (`was_overridden`).
// - Conflictos en cola: si ya hay una orden para el mismo pasaje, gana la de mayor prioridad
//   (en empate, la que ya estaba).
// - Tope: al superar `capacity`, se descarta la orden de menor prioridad (la más nueva en empate).
// - take(): entrega hasta N órdenes, la de mayor prioridad primero; FIFO dentro de la misma prioridad.
// - La validación de seguridad (p. ej. no encerrar a nadie) se hace al APLICAR, no aquí.
// Lógica pura: sin Godot. Ver docs/ARQUITECTURA.md §17.
class MapChangeQueue {
public:
	explicit MapChangeQueue(int p_capacity = 64) :
			capacity_(p_capacity) {}

	// Devuelve false si la orden fue rechazada (reserva o cola con algo más prioritario para ese pasaje).
	bool push(const MapChangeOrder &p_order);
	// Hasta p_max órdenes, por prioridad (mayor primero) y orden de llegada.
	std::vector<MapChangeOrder> take(int p_max);

	bool reserve(int p_a, int p_b, int p_source, ChangePriority p_priority);
	void release(int p_a, int p_b, int p_source);
	// ¿La reserva de p_source sobre este pasaje fue pisada por alguien más prioritario?
	bool is_reserved_by(int p_a, int p_b, int p_source) const;

	void clear();
	int size() const { return static_cast<int>(orders_.size()); }
	bool empty() const { return orders_.empty(); }
	int dropped_count() const { return dropped_; } // órdenes descartadas por tope o conflicto

private:
	struct Reservation {
		int source;
		ChangePriority priority;
	};

	int capacity_;
	std::deque<MapChangeOrder> orders_;
	std::unordered_map<int64_t, Reservation> reservations_;
	int dropped_ = 0;

	static int64_t key(int p_a, int p_b);
	bool blocked_by_reservation(const MapChangeOrder &p_order) const;
};

} // namespace lv::core
