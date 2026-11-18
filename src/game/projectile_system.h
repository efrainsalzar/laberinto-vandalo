#pragma once

#include "core/health.h"
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
// - hold_beam(): rayo sostenido (láser): el arma lo renueva cada frame mientras se mantiene el gatillo; le pega
//   en el acto al primer blanco que toca. Si un frame no se renueva, se apaga.
// - Cada frame de física: avanza las balas contra las paredes del Maze y contra los participantes que son
//   blanco ahora (en juego y sin protección; nunca quien disparó). Si la bala explota (bazuca), le pega a todos
//   los del radio con línea de visión. Cada golpe es un lv::core::DamageInfo con el daño del arma (WeaponType):
//   bala/perdigón = damage; láser = damage × delta por frame; explosión = de damage (centro) a damage_edge (borde).
//   Se aplica directo al HealthComponent de la víctima en C++ (§25), sin pasar por GDScript; si el participante
//   no tiene salud, emite participant_hit(victim, by) y quien escucha decide. exploded(...) para la vista.
// - Antes de la salud, el golpe pasa por los escudos de la víctima (ShieldComponent, §27). Si un espejo lo
//   devuelve: la bala vuelve como bala nueva de la víctima; el láser quema directo a quien lo disparó.
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
	// p_delta: segundos de este frame (el láser quita damage × delta).
	void hold_beam(Participant *p_owner, const Vector2 &p_from, const Vector2 &p_direction, const Ref<WeaponType> &p_weapon, float p_range, float p_delta);
	void clear();
	int get_projectile_count() const { return world_.size(); }

	void set_max_projectiles(int p_value);
	int get_max_projectiles() const { return world_.max(); }

protected:
	static void _bind_methods();

private:
	static constexpr float TRAIL_SECONDS = 0.03f; // largo de la estela = lo que la bala recorre en este tiempo
	static constexpr float EXPLOSION_SECONDS = 0.35f;
	static constexpr float REFLECT_RANGE = 12.0f; // casillas que recorre una bala devuelta

	// Explosión que se apaga sola. En coordenadas locales.
	struct Flash {
		Vector2 from;
		float radius = 0.0f; // px
		Color color;
		float time = 0.0f;
		float duration = 0.1f;
	};

	// Rayo sostenido de un tirador. Vive mientras su arma lo renueve cada frame.
	struct HeldBeam {
		int owner = -1;
		Vector2 from; // local
		Vector2 to; // local: pared, blanco o alcance
		Color color;
		float width = 2.0f; // px
		bool refreshed = false;
	};

	Maze *maze_ = nullptr;
	lv::core::ProjectileWorld world_;
	std::vector<Ref<WeaponType>> weapons_; // tag de la bala = índice aquí (para dibujarla con su color)
	std::vector<lv::core::ProjectileEvent> events_; // reutilizado cada frame
	std::vector<lv::core::ProjectileTarget> targets_;
	std::vector<Participant *> bodies_; // bodies_[i] es el participante de id i (o nullptr)
	std::vector<lv::core::ExplosionVictim> victims_;
	std::vector<Flash> flashes_;
	std::vector<HeldBeam> held_beams_;

	int weapon_tag(const Ref<WeaponType> &p_weapon);
	Color weapon_color(int p_tag) const;
	void gather_targets();
	const lv::core::WeaponSpec *weapon_spec(int p_tag) const;
	// p_tag: arma de la bala (para devolverla con un espejo); -1 = no es una bala.
	void hit(int p_victim, const lv::core::DamageInfo &p_info, int p_tag = -1);
	void reflect(Participant *p_mirror, const lv::core::DamageInfo &p_info, int p_tag);
	Participant *body(int p_id) const;
	Vector2 grid_to_local(float p_x, float p_y) const;
};
