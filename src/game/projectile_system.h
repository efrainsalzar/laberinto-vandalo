#pragma once

#include "core/projectiles.h"
#include "game/weapon_type.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

using namespace godot;

class Maze;
class Participant;

// Todas las balas, rayos y explosiones de la partida en UN nodo (no un nodo por bala). Adapta
// lv::core::ProjectileWorld, lv::core::trace_beam y lv::core::explosion_victims:
// - spawn(): una bala (WeaponComponent llama una vez por perdigón).
// - fire_beam(): rayo instantáneo (láser): le pega en el acto al primer blanco.
// - Cada frame de física: avanza las balas contra las paredes del Maze y contra los participantes que son
//   blanco ahora (en juego y sin protección; nunca quien disparó). Si la bala explota (bazuca), le pega a todos
//   los del radio con línea de visión. Emite participant_hit(victim, by) por cada uno; qué hace el impacto lo
//   decide quien escucha (hoy match.gd: elimina y, si llevaba el trofeo, lo suelta). exploded(...) para la vista.
// - Dibuja todo en un solo _draw (arte provisional): balas con estela, rayos y explosiones que se apagan.
// - Tope `max_projectiles`: al superarlo, la bala nueva reemplaza a la más vieja.
// Hay uno por nivel; WeaponComponent lo encuentra con ProjectileSystem::find (grupo "projectile_system").
// Ver docs/ARQUITECTURA.md §22 y §23.
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
	void fire_beam(Participant *p_owner, const Vector2 &p_from, const Vector2 &p_direction, const Ref<WeaponType> &p_weapon, float p_range);
	void clear();
	int get_projectile_count() const { return world_.size(); }

	void set_max_projectiles(int p_value);
	int get_max_projectiles() const { return world_.max(); }

protected:
	static void _bind_methods();

private:
	static constexpr float TRAIL_SECONDS = 0.03f; // largo de la estela = lo que la bala recorre en este tiempo
	static constexpr float BEAM_SECONDS = 0.12f;
	static constexpr float EXPLOSION_SECONDS = 0.35f;

	// Efecto visual que se apaga solo (rayo o explosión). En coordenadas locales.
	struct Flash {
		Vector2 from;
		Vector2 to; // rayo: fin; explosión: sin uso
		float radius = 0.0f; // explosión: radio en px
		Color color;
		float time = 0.0f;
		float duration = 0.1f;
		bool beam = false;
	};

	Maze *maze_ = nullptr;
	lv::core::ProjectileWorld world_;
	std::vector<Ref<WeaponType>> weapons_; // tag de la bala = índice aquí (para dibujarla con su color)
	std::vector<lv::core::ProjectileEvent> events_; // reutilizado cada frame
	std::vector<lv::core::ProjectileTarget> targets_;
	std::vector<Participant *> bodies_; // bodies_[i] es el participante de id i (o nullptr)
	std::vector<int> victims_;
	std::vector<Flash> flashes_;

	int weapon_tag(const Ref<WeaponType> &p_weapon);
	Color weapon_color(int p_tag) const;
	void gather_targets();
	void hit(int p_victim, int p_owner);
	Participant *body(int p_id) const;
	Vector2 grid_to_local(float p_x, float p_y) const;
};
