#pragma once

#include "core/projectiles.h"

#include <cstdint>
#include <vector>

namespace lv::core {

enum class ExplosiveKind {
	Bomb, // explota sola al terminar la mecha
	Mine, // se arma y explota cuando un rival pasa cerca
};

// Datos de un explosivo (Type Object, como WeaponSpec). En Godot vienen de un UsableItem (.tres). Casillas y segundos.
struct ExplosiveSpec {
	ExplosiveKind kind = ExplosiveKind::Bomb;
	float fuse = 2.0f; // bomba: segundos hasta explotar
	float arm_time = 1.0f; // mina: segundos hasta quedar armada
	float trigger_radius = 0.8f; // mina: un rival a esta distancia (más su cuerpo) la hace explotar
	float explosion_radius = 2.0f;
};

// Un explosivo en el piso.
struct Explosive {
	float x = 0.0f;
	float y = 0.0f;
	int owner = -1; // quien lo puso (la mina nunca explota por él)
	int tag = 0; // dato libre para quien lo usa (qué objeto, para el daño y el dibujo)
	ExplosiveSpec spec;
	float timer = 0.0f; // bomba: mecha que queda; mina: tiempo para armarse
	float age = 0.0f; // segundos en el piso (para el parpadeo de la vista)
	uint32_t seq = 0; // orden de creación (para quitar el más viejo)

	bool is_armed() const { return spec.kind == ExplosiveKind::Mine && timer <= 0.0f; }
};

struct ExplosiveEvent {
	float x = 0.0f;
	float y = 0.0f;
	int owner = -1;
	int tag = 0;
	float explosion_radius = 0.0f;
	ExplosiveKind kind = ExplosiveKind::Bomb;
	int trigger = -1; // mina: quién la pisó
};

// Todas las bombas y minas del mapa en un arreglo (datos en bloque, como ProjectileWorld): sin un nodo por bomba.
// - place(): pone uno; con el tope lleno, desaparece el más viejo (nunca crece sin límite).
// - step(): bombas descuentan la mecha; minas se arman y, armadas, explotan si un blanco que no es su dueño
//   las toca. Lo que explota sale como evento (la explosión y su daño los aplica quien llama).
// Orden fijo (por posición en el arreglo): resultado reproducible. Ver docs/ARQUITECTURA.md §28.
class ExplosiveWorld {
public:
	explicit ExplosiveWorld(int p_max = 32) { set_max(p_max); }

	void set_max(int p_max);
	int max() const { return max_; }

	void place(float p_x, float p_y, const ExplosiveSpec &p_spec, int p_owner, int p_tag = 0);
	void step(float p_delta, const std::vector<ProjectileTarget> &p_targets, std::vector<ExplosiveEvent> &r_events);
	void clear() { items_.clear(); }

	const std::vector<Explosive> &items() const { return items_; }
	int size() const { return static_cast<int>(items_.size()); }

private:
	std::vector<Explosive> items_;
	int max_ = 32;
	uint32_t next_seq_ = 0;
};

} // namespace lv::core
