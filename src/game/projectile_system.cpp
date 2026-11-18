#include "game/projectile_system.h"

#include "core/perf.h"
#include "game/maze.h"
#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <algorithm>
#include <cmath>

namespace {
const char *GROUP_SELF = "projectile_system";
}

void ProjectileSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "maze"), &ProjectileSystem::setup);
	ClassDB::bind_method(D_METHOD("clear"), &ProjectileSystem::clear);
	ClassDB::bind_method(D_METHOD("get_projectile_count"), &ProjectileSystem::get_projectile_count);
	ClassDB::bind_method(D_METHOD("get_explosive_count"), &ProjectileSystem::get_explosive_count);
	ClassDB::bind_method(D_METHOD("set_viewer", "viewer"), &ProjectileSystem::set_viewer);
	ClassDB::bind_method(D_METHOD("place_explosive", "owner", "item"), &ProjectileSystem::place_explosive);
	ClassDB::bind_method(D_METHOD("set_max_projectiles", "value"), &ProjectileSystem::set_max_projectiles);
	ClassDB::bind_method(D_METHOD("get_max_projectiles"), &ProjectileSystem::get_max_projectiles);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_projectiles", PROPERTY_HINT_RANGE, "1,4096,1"), "set_max_projectiles", "get_max_projectiles");

	ADD_SIGNAL(MethodInfo("participant_hit",
			PropertyInfo(Variant::OBJECT, "victim", PROPERTY_HINT_NODE_TYPE, "Participant"),
			PropertyInfo(Variant::OBJECT, "by", PROPERTY_HINT_NODE_TYPE, "Participant")));
	ADD_SIGNAL(MethodInfo("exploded", PropertyInfo(Variant::VECTOR2, "world_position"), PropertyInfo(Variant::FLOAT, "radius_px")));
}

ProjectileSystem *ProjectileSystem::find(SceneTree *p_tree) {
	if (p_tree == nullptr) {
		return nullptr;
	}
	return Object::cast_to<ProjectileSystem>(p_tree->get_first_node_in_group(GROUP_SELF));
}

void ProjectileSystem::_ready() {
	add_to_group(GROUP_SELF);
	set_physics_process(!Engine::get_singleton()->is_editor_hint());
}

void ProjectileSystem::setup(Maze *p_maze) {
	maze_ = p_maze;
	clear();
}

void ProjectileSystem::clear() {
	world_.clear();
	explosives_.clear();
	flashes_.clear();
	held_beams_.clear();
	queue_redraw();
}

void ProjectileSystem::set_max_projectiles(int p_value) {
	world_.set_max(p_value);
}

int ProjectileSystem::item_tag(const Ref<UsableItem> &p_item) {
	for (size_t i = 0; i < items_.size(); i++) {
		if (items_[i] == p_item) {
			return static_cast<int>(i);
		}
	}
	items_.push_back(p_item);
	return static_cast<int>(items_.size()) - 1;
}

void ProjectileSystem::set_viewer(Participant *p_viewer) {
	viewer_id_ = p_viewer != nullptr ? p_viewer->get_participant_id() : -1;
}

void ProjectileSystem::place_explosive(Participant *p_owner, const Ref<UsableItem> &p_item) {
	if (maze_ == nullptr || p_owner == nullptr || p_item.is_null()) {
		return;
	}
	const lv::core::GridPoint at = maze_->world_to_grid_point(p_owner->get_global_position());
	explosives_.place(at.x, at.y, p_item->spec(), p_owner->get_participant_id(), item_tag(p_item));
	queue_redraw();
}

void ProjectileSystem::explode(float p_x, float p_y, float p_radius, int p_owner, float p_damage, float p_edge, const Color &p_color) {
	const float tile = maze_->get_tile_px();
	victims_.clear();
	lv::core::explosion_victims(maze_->get_grid(), p_x, p_y, p_radius, targets_, p_owner, victims_);
	Flash flash;
	flash.from = grid_to_local(p_x, p_y);
	flash.radius = p_radius * tile;
	flash.color = p_color;
	flash.duration = EXPLOSION_SECONDS;
	flashes_.push_back(flash);
	emit_signal("exploded", to_global(flash.from), flash.radius);
	for (const lv::core::ExplosionVictim &victim : victims_) {
		lv::core::DamageInfo info;
		info.amount = lv::core::explosion_damage(p_damage, p_edge, p_radius, victim.distance);
		info.source = p_owner;
		info.kind = lv::core::DamageKind::Explosion;
		info.x = p_x;
		info.y = p_y;
		if (const Participant *p = body(victim.id)) { // empuja hacia afuera del centro
			const lv::core::GridPoint at = maze_->world_to_grid_point(p->get_global_position());
			const Vector2 away = Vector2(at.x - p_x, at.y - p_y).normalized();
			info.dir_x = away.x;
			info.dir_y = away.y;
		}
		hit(victim.id, info);
	}
}

int ProjectileSystem::weapon_tag(const Ref<WeaponType> &p_weapon) {
	for (size_t i = 0; i < weapons_.size(); i++) {
		if (weapons_[i] == p_weapon) {
			return static_cast<int>(i);
		}
	}
	weapons_.push_back(p_weapon);
	return static_cast<int>(weapons_.size()) - 1;
}

Color ProjectileSystem::weapon_color(int p_tag) const {
	return (p_tag >= 0 && p_tag < static_cast<int>(weapons_.size())) ? weapons_[p_tag]->get_projectile_color() : Color(1, 1, 1);
}

void ProjectileSystem::spawn(Participant *p_owner, const Vector2 &p_from, const Vector2 &p_direction, const Ref<WeaponType> &p_weapon, float p_range) {
	if (maze_ == nullptr || p_owner == nullptr || p_weapon.is_null()) {
		return;
	}
	const lv::core::GridPoint from = maze_->world_to_grid_point(p_from);
	lv::core::WeaponSpec spec = p_weapon->spec();
	spec.range = p_range;
	world_.spawn(from.x, from.y, p_direction.x, p_direction.y, spec, p_owner->get_participant_id(), weapon_tag(p_weapon));
}

void ProjectileSystem::hold_beam(Participant *p_owner, const Vector2 &p_from, const Vector2 &p_direction, const Ref<WeaponType> &p_weapon, float p_range, float p_delta) {
	if (maze_ == nullptr || p_owner == nullptr || p_weapon.is_null()) {
		return;
	}
	lv::core::PerfScope perf("weapons.beam");
	gather_targets();
	const int owner = p_owner->get_participant_id();
	const lv::core::GridPoint from = maze_->world_to_grid_point(p_from);
	const lv::core::BeamHit result = lv::core::trace_beam(maze_->get_grid(), from.x, from.y, p_direction.x, p_direction.y, p_range, targets_, owner);
	HeldBeam *beam = nullptr;
	for (HeldBeam &b : held_beams_) {
		if (b.owner == owner) {
			beam = &b;
			break;
		}
	}
	if (beam == nullptr) {
		held_beams_.push_back(HeldBeam());
		beam = &held_beams_.back();
		beam->owner = owner;
	}
	beam->from = grid_to_local(from.x, from.y);
	beam->to = grid_to_local(result.end_x, result.end_y);
	beam->color = p_weapon->get_projectile_color();
	beam->width = std::max(1.5f, p_weapon->get_projectile_radius() * maze_->get_tile_px());
	beam->refreshed = true;
	queue_redraw();
	if (result.target >= 0) {
		lv::core::DamageInfo info;
		info.amount = p_weapon->get_damage() * p_delta;
		info.source = owner;
		info.kind = lv::core::DamageKind::Beam;
		info.dir_x = p_direction.x;
		info.dir_y = p_direction.y;
		info.x = result.end_x;
		info.y = result.end_y;
		hit(result.target, info);
	}
}

Participant *ProjectileSystem::body(int p_id) const {
	return (p_id >= 0 && p_id < static_cast<int>(bodies_.size())) ? bodies_[p_id] : nullptr;
}

const lv::core::WeaponSpec *ProjectileSystem::weapon_spec(int p_tag) const {
	return (p_tag >= 0 && p_tag < static_cast<int>(weapons_.size())) ? &weapons_[p_tag]->spec() : nullptr;
}

void ProjectileSystem::hit(int p_victim, const lv::core::DamageInfo &p_info, int p_tag) {
	Participant *victim = body(p_victim);
	if (victim == nullptr) {
		return;
	}
	// Escudos primero (cadena: burbuja → frontal → espejo). La salud solo se entera si el golpe pasa.
	if (ShieldComponent *shields = victim->get_shields()) {
		const lv::core::ShieldResult shield = shields->filter(p_info);
		if (shield.outcome == lv::core::ShieldOutcome::Reflected) {
			reflect(victim, p_info, p_tag);
			return;
		}
		if (shield.outcome == lv::core::ShieldOutcome::Absorbed) {
			return;
		}
	}
	if (HealthComponent *health = victim->get_health()) {
		health->take_damage(p_info, body(p_info.source));
	} else {
		emit_signal("participant_hit", victim, body(p_info.source)); // sin salud: decide quien escucha
	}
}

// Espejo: el golpe vuelve por donde vino, ahora de quien tenía el espejo (y marcado: otro espejo no lo devuelve).
void ProjectileSystem::reflect(Participant *p_mirror, const lv::core::DamageInfo &p_info, int p_tag) {
	lv::core::DamageInfo back = p_info;
	back.source = p_mirror->get_participant_id();
	back.dir_x = -p_info.dir_x;
	back.dir_y = -p_info.dir_y;
	back.reflected = true;
	if (p_info.kind == lv::core::DamageKind::Beam) {
		if (p_info.source >= 0 && p_info.source != back.source) {
			hit(p_info.source, back); // el láser quema a quien lo dispara
		}
		return;
	}
	const lv::core::WeaponSpec *spec = weapon_spec(p_tag);
	if (spec == nullptr) {
		return;
	}
	lv::core::WeaponSpec copy = *spec;
	copy.range = REFLECT_RANGE;
	const lv::core::GridPoint at = maze_->world_to_grid_point(p_mirror->get_global_position());
	world_.spawn(at.x, at.y, back.dir_x, back.dir_y, copy, back.source, p_tag, true);
}

// Blancos: participantes en juego y sin protección (en casillas). También arma bodies_ (id -> participante).
void ProjectileSystem::gather_targets() {
	const float tile = maze_->get_tile_px();
	targets_.clear();
	bodies_.clear();
	const TypedArray<Node> nodes = get_tree()->get_nodes_in_group("participants");
	for (int i = 0; i < nodes.size(); i++) {
		Participant *p = Object::cast_to<Participant>(nodes[i]);
		if (p == nullptr) {
			continue;
		}
		const int id = p->get_participant_id();
		if (id >= 0) {
			if (id >= static_cast<int>(bodies_.size())) {
				bodies_.resize(id + 1, nullptr);
			}
			bodies_[id] = p;
		}
		if (!p->is_vulnerable()) {
			continue;
		}
		const lv::core::GridPoint at = maze_->world_to_grid_point(p->get_global_position());
		targets_.push_back({ id, at.x, at.y, p->get_body_radius() / tile });
	}
}

void ProjectileSystem::_physics_process(double p_delta) {
	if (maze_ == nullptr || (world_.size() == 0 && explosives_.size() == 0 && flashes_.empty() && held_beams_.empty())) {
		return; // nada que mover ni dibujar (el último frame ya se redibujó vacío)
	}
	lv::core::PerfScope perf("weapons.projectiles");
	const float dt = static_cast<float>(p_delta);

	// Rayos sostenidos: se apagan los que su arma no renovó desde la última vez (gatillo suelto, sin batería,
	// tirador eliminado). Los que quedan deben renovarse de nuevo antes de la próxima.
	held_beams_.erase(std::remove_if(held_beams_.begin(), held_beams_.end(), [](const HeldBeam &b) { return !b.refreshed; }), held_beams_.end());
	for (HeldBeam &beam : held_beams_) {
		beam.refreshed = false;
	}

	// Efectos que se apagan.
	for (Flash &flash : flashes_) {
		flash.time += dt;
	}
	flashes_.erase(std::remove_if(flashes_.begin(), flashes_.end(), [](const Flash &f) { return f.time >= f.duration; }), flashes_.end());

	if (world_.size() > 0 || explosives_.size() > 0) {
		gather_targets();
	}
	if (world_.size() > 0) {
		events_.clear();
		world_.step(dt, maze_->get_grid(), targets_, events_);
		for (const lv::core::ProjectileEvent &event : events_) {
			if (event.explosion_radius > 0.0f) {
				// Bazuca: explota donde terminó y le pega a todos los del radio con línea de visión.
				const lv::core::WeaponSpec *spec = weapon_spec(event.tag);
				if (spec != nullptr) {
					explode(event.x, event.y, event.explosion_radius, event.owner, spec->damage, spec->damage_edge, weapon_color(event.tag));
				}
			} else if (event.kind == lv::core::ProjectileEvent::Kind::HitTarget) {
				const lv::core::WeaponSpec *spec = weapon_spec(event.tag);
				lv::core::DamageInfo info;
				info.amount = spec != nullptr ? spec->damage : 0.0f;
				info.source = event.owner;
				info.kind = lv::core::DamageKind::Bullet;
				info.dir_x = event.dir_x;
				info.dir_y = event.dir_y;
				info.x = event.x;
				info.y = event.y;
				info.reflected = event.reflected;
				hit(event.target, info, event.tag);
			}
		}
	}
	if (explosives_.size() > 0) {
		lv::core::PerfScope perf_explosives("weapons.explosives");
		explosive_events_.clear();
		explosives_.step(dt, targets_, explosive_events_);
		for (const lv::core::ExplosiveEvent &event : explosive_events_) {
			const Ref<UsableItem> item = (event.tag >= 0 && event.tag < static_cast<int>(items_.size())) ? items_[event.tag] : Ref<UsableItem>();
			if (item.is_valid()) {
				explode(event.x, event.y, event.explosion_radius, event.owner, item->get_damage(), item->get_damage_edge(), item->get_color());
			}
		}
	}
	queue_redraw();
}

Vector2 ProjectileSystem::grid_to_local(float p_x, float p_y) const {
	const float tile = maze_->get_tile_px();
	return to_local(maze_->to_global(Vector2(p_x, p_y) * tile));
}

void ProjectileSystem::_draw() {
	if (maze_ == nullptr) {
		return;
	}
	const float tile = maze_->get_tile_px();
	for (const lv::core::Projectile &p : world_.projectiles()) {
		const Color color = weapon_color(p.tag);
		const float radius = p.radius * tile > 1.0f ? p.radius * tile : 1.0f;
		const Vector2 at = grid_to_local(p.x, p.y);
		// Estela tipo bala trazadora: lo que recorrió en los últimos TRAIL_SECONDS (nunca sale del tirador).
		const float trail_tiles = std::min(p.traveled, p.speed * TRAIL_SECONDS);
		const Vector2 tail = at - Vector2(p.dir_x, p.dir_y) * trail_tiles * tile;
		draw_line(tail, at, Color(color, 0.35f), radius * 2.0f);
		draw_line(tail.lerp(at, 0.5f), at, Color(color, 0.8f), radius);
		draw_circle(at, radius, color);
	}
	draw_explosives();
	for (const Flash &flash : flashes_) {
		const float fade = 1.0f - flash.time / flash.duration;
		const float grow = 0.4f + 0.6f * (1.0f - fade * fade); // crece rápido al principio
		draw_circle(flash.from, flash.radius * grow, Color(flash.color, 0.35f * fade));
		draw_arc(flash.from, flash.radius * grow, 0.0f, 6.2831853f, 40, Color(1, 1, 1, 0.8f * fade), 2.0f);
	}
	// Rayos sostenidos: un halo ancho del color del arma, el cuerpo del color y un núcleo blanco; brillo en la punta.
	for (const HeldBeam &beam : held_beams_) {
		draw_line(beam.from, beam.to, Color(beam.color, 0.22f), beam.width * 4.0f);
		draw_line(beam.from, beam.to, Color(beam.color, 0.85f), beam.width * 1.8f);
		draw_line(beam.from, beam.to, Color(1, 1, 1, 0.95f), beam.width * 0.7f);
		draw_circle(beam.to, beam.width * 2.2f, Color(beam.color, 0.5f));
		draw_circle(beam.to, beam.width * 1.1f, Color(1, 1, 1, 0.9f));
	}
}

// Bombas y minas (arte provisional). Bomba: esfera oscura con anillo del color y una chispa en la mecha que parpadea
// cada vez más rápido. Mina: disco chato; luz tenue mientras se arma y roja parpadeando lenta ya armada. Las minas
// ajenas solo se dibujan si el jugador de esta pantalla está muy cerca (la niebla, además, las tapa de lejos).
void ProjectileSystem::draw_explosives() {
	if (explosives_.size() == 0) {
		return;
	}
	const float tile = maze_->get_tile_px();
	const Participant *viewer = body(viewer_id_);
	lv::core::GridPoint viewer_at{ -1000.0f, -1000.0f };
	if (viewer != nullptr) {
		viewer_at = maze_->world_to_grid_point(viewer->get_global_position());
	}
	for (const lv::core::Explosive &e : explosives_.items()) {
		const Ref<UsableItem> item = (e.tag >= 0 && e.tag < static_cast<int>(items_.size())) ? items_[e.tag] : Ref<UsableItem>();
		const Color color = item.is_valid() ? item->get_color() : Color(1, 0.5f, 0.2f);
		const Vector2 at = grid_to_local(e.x, e.y);
		const float r = std::max(3.0f, 0.24f * tile);
		if (e.spec.kind == lv::core::ExplosiveKind::Bomb) {
			const float left = std::max(0.0f, e.timer);
			const float period = 0.08f + 0.3f * std::min(1.0f, left / std::max(0.01f, e.spec.fuse));
			const bool spark = std::fmod(e.age, period * 2.0f) < period;
			draw_circle(at, r + 1.5f, Color(0.02f, 0.02f, 0.04f, 0.9f));
			draw_circle(at, r, Color(0.16f, 0.15f, 0.2f));
			draw_arc(at, r, 0.0f, 6.2831853f, 20, color, 1.5f);
			const Vector2 fuse = at + Vector2(r * 0.55f, -r * 0.95f);
			draw_line(at + Vector2(r * 0.3f, -r * 0.7f), fuse, Color(0.8f, 0.75f, 0.6f), 1.5f);
			if (spark) {
				draw_circle(fuse, r * 0.45f, Color(1.0f, 0.85f, 0.3f));
				draw_circle(fuse, r * 0.22f, Color(1, 1, 1));
			}
			// Anillo del radio de la explosión que se cierra en el último medio segundo (aviso).
			if (left < 0.5f) {
				draw_arc(at, e.spec.explosion_radius * tile, 0.0f, 6.2831853f, 40, Color(color, 0.5f * (1.0f - left / 0.5f)), 1.5f);
			}
		} else {
			float alpha = 1.0f;
			if (e.owner != viewer_id_) {
				const float dx = viewer_at.x - e.x;
				const float dy = viewer_at.y - e.y;
				if (dx * dx + dy * dy > MINE_REVEAL * MINE_REVEAL) {
					continue; // mina ajena lejos: no se ve
				}
			} else {
				alpha = 0.85f;
			}
			draw_circle(at, r * 1.15f + 1.5f, Color(0.02f, 0.02f, 0.04f, 0.8f * alpha));
			draw_circle(at, r * 1.15f, Color(0.2f, 0.19f, 0.24f, alpha));
			draw_arc(at, r * 1.15f, 0.0f, 6.2831853f, 20, Color(color, 0.8f * alpha), 1.5f);
			const bool armed = e.is_armed();
			const bool on = !armed || std::fmod(e.age, 1.0f) < 0.5f;
			const Color light = armed ? Color(1.0f, 0.2f, 0.15f, alpha) : Color(color, 0.45f * alpha);
			if (on) {
				draw_circle(at, r * 0.35f, light);
			}
		}
	}
}
