#include "game/projectile_system.h"

#include "core/perf.h"
#include "game/maze.h"
#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/typed_array.hpp>

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

void ProjectileSystem::spawn(Participant *p_owner, const Vector2 &p_from, const Vector2 &p_direction, const Ref<WeaponType> &p_weapon) {
	if (maze_ == nullptr || p_owner == nullptr || p_weapon.is_null()) {
		return;
	}
	const lv::core::GridPoint from = maze_->world_to_grid_point(p_from);
	world_.spawn(from.x, from.y, p_direction.x, p_direction.y, p_weapon->spec(), p_owner->get_participant_id(), weapon_tag(p_weapon));
}

Participant *ProjectileSystem::body(int p_id) const {
	return (p_id >= 0 && p_id < static_cast<int>(bodies_.size())) ? bodies_[p_id] : nullptr;
}

void ProjectileSystem::_physics_process(double p_delta) {
	if (maze_ == nullptr) {
		return;
	}
	if (world_.size() == 0) {
		return; // nada que mover ni dibujar (el último frame ya se redibujó vacío)
	}
	lv::core::PerfScope perf("weapons.projectiles");

	// Blancos: participantes en juego y sin protección (en casillas).
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

	events_.clear();
	world_.step(static_cast<float>(p_delta), maze_->get_grid(), targets_, events_);
	for (const lv::core::ProjectileEvent &event : events_) {
		if (event.kind != lv::core::ProjectileEvent::Kind::HitTarget) {
			continue;
		}
		Participant *victim = body(event.target);
		if (victim != nullptr) {
			emit_signal("participant_hit", victim, body(event.owner));
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
		const Color color = (p.tag >= 0 && p.tag < static_cast<int>(weapons_.size())) ? weapons_[p.tag]->get_projectile_color() : Color(1, 1, 1);
		const float radius = p.radius * tile > 1.5f ? p.radius * tile : 1.5f;
		const Vector2 at = grid_to_local(p.x, p.y);
		// Estela corta hacia atrás para que se lea la dirección.
		draw_line(at - Vector2(p.dir_x, p.dir_y) * radius * 3.0f, at, Color(color, 0.5f), radius);
		draw_circle(at, radius, color);
	}
}
