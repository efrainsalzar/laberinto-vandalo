#pragma once

#include "core/maze_grid.h"
#include "core/weapon.h"

#include <cstdint>
#include <vector>

namespace lv::core {

// Una bala en vuelo. Todo en casillas (el centro de la casilla (2, 3) es (2.5, 3.5)).
struct Projectile {
	float x = 0.0f;
	float y = 0.0f;
	float dir_x = 1.0f; // dirección normalizada
	float dir_y = 0.0f;
	float speed = 0.0f; // casillas/s
	float distance_left = 0.0f; // casillas que le quedan antes de desaparecer
	float traveled = 0.0f; // casillas recorridas (para dibujar la estela sin que salga del tirador)
	float radius = 0.1f;
	int owner = -1; // quien la disparó (nunca le pega a él)
	int tag = 0; // dato libre para quien la usa (p. ej. qué arma, para dibujarla)
	uint32_t seq = 0; // orden de creación (para reemplazar la más vieja)
};

// Algo a lo que una bala le puede pegar (un círculo). Quien llama solo pasa los que son blanco ahora
// (p. ej. participantes vivos y sin protección).
struct ProjectileTarget {
	int id = -1;
	float x = 0.0f;
	float y = 0.0f;
	float radius = 0.4f;
};

struct ProjectileEvent {
	enum class Kind {
		HitWall,
		HitTarget,
		Expired, // llegó a su alcance sin pegarle a nada
	};
	Kind kind = Kind::Expired;
	float x = 0.0f; // dónde terminó
	float y = 0.0f;
	int owner = -1;
	int target = -1; // solo HitTarget
	int tag = 0;
};

// Todas las balas del mapa como datos en bloque (un std::vector de structs), no un nodo por bala.
// - spawn(): si se llegó al tope, la bala nueva reemplaza a la más vieja (nunca crece sin límite).
// - step(): avanza todas; cada una recorre su tramo del frame contra paredes (DDA: no atraviesa paredes
//   aunque vaya rápido) y contra los blancos (segmento contra círculo). Gana lo que toque primero.
// - Las que terminan se quitan intercambiándolas con la última (swap-remove): sin pedir memoria nueva.
//   Es la versión "de juegos" del Object Pool: un arreglo que se reutiliza.
// Ver docs/ARQUITECTURA.md §22.
class ProjectileWorld {
public:
	explicit ProjectileWorld(int p_max = 256) { set_max(p_max); }

	void set_max(int p_max);
	int max() const { return max_; }

	// Dispara una bala desde (x, y) hacia (dir_x, dir_y). Devuelve false si la dirección es (0, 0).
	bool spawn(float p_x, float p_y, float p_dir_x, float p_dir_y, const WeaponSpec &p_spec, int p_owner, int p_tag = 0);
	// Avanza p_delta segundos. Agrega a r_events lo que terminó (en orden fijo: resultado reproducible).
	void step(float p_delta, const MazeGrid &p_grid, const std::vector<ProjectileTarget> &p_targets, std::vector<ProjectileEvent> &r_events);
	void clear() { items_.clear(); }

	const std::vector<Projectile> &projectiles() const { return items_; }
	int size() const { return static_cast<int>(items_.size()); }

private:
	std::vector<Projectile> items_;
	int max_ = 256;
	uint32_t next_seq_ = 0;
};

} // namespace lv::core
