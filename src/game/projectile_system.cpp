#include "game/projectile_system.h"

#include "core/perf.h"
#include "game/maze.h"
#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <algorithm>

namespace {
const char *GROUP_SELF = "projectile_system";
}

void ProjectileSystem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "maze"), &ProjectileSystem::setup);
	ClassDB::bind_method(D_METHOD("clear"), &ProjectileSystem::clear);
	ClassDB::bind_method(D_METHOD("get_projectile_count"), &ProjectileSystem::get_projectile_count);
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
	flashes_.clear();
	queue_redraw();
}

void ProjectileSystem::set_max_projectiles(int p_value) {
	world_.set_max(p_value);
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

void ProjectileSystem::fire_beam(Participant *p_owner, const Vector2 &p_from, const Vector2 &p_direction, const Ref<WeaponType> &p_weapon, float p_range) {
	if (maze_ == nullptr || p_owner == nullptr || p_weapon.is_null()) {
		return;
	}
	lv::core::PerfScope perf("weapons.beam");
	gather_targets();
	const lv::core::GridPoint from = maze_->world_to_grid_point(p_from);
	const lv::core::BeamHit result = lv::core::trace_beam(maze_->get_grid(), from.x, from.y, p_direction.x, p_direction.y, p_range, targets_, p_owner->get_participant_id());
	Flash flash;
	flash.beam = true;
	flash.from = grid_to_local(from.x, from.y);
	flash.to = grid_to_local(result.end_x, result.end_y);
	flash.color = p_weapon->get_projectile_color();
	flash.duration = BEAM_SECONDS;
	flash.radius = std::max(1.5f, p_weapon->get_projectile_radius() * maze_->get_tile_px());
	flashes_.push_back(flash);
	queue_redraw();
	if (result.target >= 0) {
		hit(result.target, p_owner->get_participant_id());
	}
}

Participant *ProjectileSystem::body(int p_id) const {
	return (p_id >= 0 && p_id < static_cast<int>(bodies_.size())) ? bodies_[p_id] : nullptr;
}

void ProjectileSystem::hit(int p_victim, int p_owner) {
	Participant *victim = body(p_victim);
	if (victim != nullptr) {
		emit_signal("participant_hit", victim, body(p_owner));
	}
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
	if (maze_ == nullptr || (world_.size() == 0 && flashes_.empty())) {
		return; // nada que mover ni dibujar (el último frame ya se redibujó vacío)
	}
	lv::core::PerfScope perf("weapons.projectiles");
	const float dt = static_cast<float>(p_delta);

	// Efectos que se apagan.
	for (Flash &flash : flashes_) {
		flash.time += dt;
	}
	flashes_.erase(std::remove_if(flashes_.begin(), flashes_.end(), [](const Flash &f) { return f.time >= f.duration; }), flashes_.end());

	if (world_.size() > 0) {
		gather_targets();
		events_.clear();
		world_.step(dt, maze_->get_grid(), targets_, events_);
		const float tile = maze_->get_tile_px();
		for (const lv::core::ProjectileEvent &event : events_) {
			if (event.explosion_radius > 0.0f) {
				// Bazuca: explota donde terminó y le pega a todos los del radio con línea de visión.
				victims_.clear();
				lv::core::explosion_victims(maze_->get_grid(), event.x, event.y, event.explosion_radius, targets_, event.owner, victims_);
				Flash flash;
				flash.from = grid_to_local(event.x, event.y);
				flash.radius = event.explosion_radius * tile;
				flash.color = weapon_color(event.tag);
				flash.duration = EXPLOSION_SECONDS;
				flashes_.push_back(flash);
				emit_signal("exploded", to_global(flash.from), flash.radius);
				for (const int victim : victims_) {
					hit(victim, event.owner);
				}
			} else if (event.kind == lv::core::ProjectileEvent::Kind::HitTarget) {
				hit(event.target, event.owner);
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
	for (const Flash &flash : flashes_) {
		const float fade = 1.0f - flash.time / flash.duration;
		if (flash.beam) {
			draw_line(flash.from, flash.to, Color(flash.color, 0.35f * fade), flash.radius * 3.0f);
			draw_line(flash.from, flash.to, Color(1, 1, 1, fade), flash.radius);
		} else {
			const float grow = 0.4f + 0.6f * (1.0f - fade * fade); // crece rápido al principio
			draw_circle(flash.from, flash.radius * grow, Color(flash.color, 0.35f * fade));
			draw_arc(flash.from, flash.radius * grow, 0.0f, 6.2831853f, 40, Color(1, 1, 1, 0.8f * fade), 2.0f);
		}
	}
}
