#pragma once

#include "core/projectiles.h"
#include "game/weapon_type.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

using namespace godot;

class Maze;
class Participant;

// Todas las balas de la partida en UN nodo (no un nodo por bala). Adapta lv::core::ProjectileWorld:
// - spawn(): lo llama WeaponComponent al disparar (posición en píxeles → casillas).
// - Cada frame de física: avanza las balas contra las paredes del Maze y contra los participantes que son
//   blanco ahora (en juego y sin protección; nunca quien disparó) y emite participant_hit(victim, by).
//   Qué hace el impacto lo decide quien escucha (hoy match.gd: elimina y, si llevaba el trofeo, lo suelta).
// - Dibuja todas las balas en un solo _draw (arte provisional: puntos del color del arma).
// - Tope `max_projectiles`: al superarlo, la bala nueva reemplaza a la más vieja.
// Hay uno por nivel; WeaponComponent lo encuentra con ProjectileSystem::find (grupo "projectile_system").
// Ver docs/ARQUITECTURA.md §22.
class ProjectileSystem : public Node2D {
	GDCLASS(ProjectileSystem, Node2D)

public:
	static ProjectileSystem *find(SceneTree *p_tree);

	void _ready() override;
	void _physics_process(double p_delta) override;
	void _draw() override;

	void setup(Maze *p_maze);
	// p_range: alcance real en casillas (WeaponComponent lo calcula; puede depender de la visión).
	void spawn(Participant *p_owner, const Vector2 &p_from, const Vector2 &p_direction, const Ref<WeaponType> &p_weapon, float p_range);
	void clear();
	int get_projectile_count() const { return world_.size(); }

	void set_max_projectiles(int p_value);
	int get_max_projectiles() const { return world_.max(); }

protected:
	static void _bind_methods();

private:
	Maze *maze_ = nullptr;
	lv::core::ProjectileWorld world_;
	std::vector<Ref<WeaponType>> weapons_; // tag de la bala = índice aquí (para dibujarla con su color)
	std::vector<lv::core::ProjectileEvent> events_; // reutilizado cada frame
	std::vector<lv::core::ProjectileTarget> targets_;
	std::vector<Participant *> bodies_; // bodies_[i] es el participante de id i (o nullptr)

	int weapon_tag(const Ref<WeaponType> &p_weapon);
	static constexpr float TRAIL_SECONDS = 0.03f; // largo de la estela = lo que la bala recorre en este tiempo

	Participant *body(int p_id) const;
	Vector2 grid_to_local(float p_x, float p_y) const;
};
