#include "core/projectiles.h"

#include "core/line_of_sight.h"

#include <cmath>

namespace lv::core {

namespace {

constexpr float WALL_BACKOFF = 0.05f; // casillas

// Primera fracción t (0..1) del segmento from→from+d que toca el círculo (c, r). -1 si no lo toca.
float segment_circle(float p_fx, float p_fy, float p_dx, float p_dy, float p_cx, float p_cy, float p_r) {
	const float ox = p_fx - p_cx;
	const float oy = p_fy - p_cy;
	const float c = ox * ox + oy * oy - p_r * p_r;
	if (c <= 0.0f) {
		return 0.0f; // ya empieza dentro
	}
	const float a = p_dx * p_dx + p_dy * p_dy;
	if (a <= 0.0f) {
		return -1.0f;
	}
	const float b = 2.0f * (ox * p_dx + oy * p_dy);
	const float disc = b * b - 4.0f * a * c;
	if (disc < 0.0f) {
		return -1.0f;
	}
	const float t = (-b - std::sqrt(disc)) / (2.0f * a);
	return (t >= 0.0f && t <= 1.0f) ? t : -1.0f;
}

} // namespace

void ProjectileWorld::set_max(int p_max) {
	max_ = p_max < 1 ? 1 : p_max;
	while (static_cast<int>(items_.size()) > max_) {
		items_.pop_back();
	}
}

bool ProjectileWorld::spawn(float p_x, float p_y, float p_dir_x, float p_dir_y, const WeaponSpec &p_spec, int p_owner, int p_tag) {
	const float len = std::sqrt(p_dir_x * p_dir_x + p_dir_y * p_dir_y);
	if (len <= 0.0f) {
		return false;
	}
	Projectile p;
	p.x = p_x;
	p.y = p_y;
	p.dir_x = p_dir_x / len;
	p.dir_y = p_dir_y / len;
	p.speed = p_spec.projectile_speed > 0.0f ? p_spec.projectile_speed : 0.0f;
	p.distance_left = p_spec.range > 0.0f ? p_spec.range : 0.0f;
	p.radius = p_spec.projectile_radius > 0.0f ? p_spec.projectile_radius : 0.0f;
	p.explosion_radius = p_spec.explosion_radius > 0.0f ? p_spec.explosion_radius : 0.0f;
	p.owner = p_owner;
	p.tag = p_tag;
	p.seq = next_seq_++;

	if (static_cast<int>(items_.size()) < max_) {
		items_.push_back(p);
		return true;
	}
	// Tope: reemplaza a la más vieja (es la que menos aporta).
	size_t oldest = 0;
	for (size_t i = 1; i < items_.size(); i++) {
		if (items_[i].seq - items_[oldest].seq > 0x80000000u) { // menor con vuelta de contador
			oldest = i;
		}
	}
	items_[oldest] = p;
	return true;
}

void ProjectileWorld::step(float p_delta, const MazeGrid &p_grid, const std::vector<ProjectileTarget> &p_targets, std::vector<ProjectileEvent> &r_events) {
	size_t i = 0;
	while (i < items_.size()) {
		Projectile &p = items_[i];
		float move = p.speed * p_delta;
		if (move > p.distance_left) {
			move = p.distance_left;
		}
		const float dx = p.dir_x * move;
		const float dy = p.dir_y * move;

		ProjectileEvent event;
		bool ended = false;
		float best_t = 2.0f;
		float wall_t = 0.0f;
		if (segment_hits_wall(p_grid, { p.x, p.y }, { p.x + dx, p.y + dy }, wall_t)) {
			best_t = wall_t;
			event.kind = ProjectileEvent::Kind::HitWall;
			ended = true;
		}
		for (const ProjectileTarget &target : p_targets) {
			if (target.id == p.owner) {
				continue;
			}
			const float t = segment_circle(p.x, p.y, dx, dy, target.x, target.y, target.radius + p.radius);
			if (t >= 0.0f && t < best_t) {
				best_t = t;
				event.kind = ProjectileEvent::Kind::HitTarget;
				event.target = target.id;
				ended = true;
			}
		}

		if (!ended) {
			p.x += dx;
			p.y += dy;
			p.distance_left -= move;
			p.traveled += move;
			if (p.distance_left > 1e-4f) {
				i++;
				continue;
			}
			event.kind = ProjectileEvent::Kind::Expired;
			best_t = 0.0f; // ya está en su punto final
		}
		if (event.kind != ProjectileEvent::Kind::HitTarget) {
			event.target = -1;
		}
		event.x = (ended ? p.x + dx * best_t : p.x);
		event.y = (ended ? p.y + dy * best_t : p.y);
		if (event.kind == ProjectileEvent::Kind::HitWall) {
			// El choque es justo en el borde de la pared: se retrocede un poco para quedar sobre el suelo.
			event.x -= p.dir_x * WALL_BACKOFF;
			event.y -= p.dir_y * WALL_BACKOFF;
		}
		event.owner = p.owner;
		event.tag = p.tag;
		event.explosion_radius = p.explosion_radius;
		r_events.push_back(event);
		items_[i] = items_.back(); // swap-remove (sin avanzar i: ahora hay otra en esta posición)
		items_.pop_back();
	}
}

BeamHit trace_beam(const MazeGrid &p_grid, float p_x, float p_y, float p_dir_x, float p_dir_y, float p_range,
		const std::vector<ProjectileTarget> &p_targets, int p_owner) {
	BeamHit hit;
	hit.end_x = p_x;
	hit.end_y = p_y;
	const float len = std::sqrt(p_dir_x * p_dir_x + p_dir_y * p_dir_y);
	if (len <= 0.0f || p_range <= 0.0f) {
		return hit;
	}
	const float dx = p_dir_x / len * p_range;
	const float dy = p_dir_y / len * p_range;
	float best_t = 1.0f;
	float wall_t = 0.0f;
	if (segment_hits_wall(p_grid, { p_x, p_y }, { p_x + dx, p_y + dy }, wall_t)) {
		best_t = wall_t;
	}
	for (const ProjectileTarget &target : p_targets) {
		if (target.id == p_owner) {
			continue;
		}
		const float t = segment_circle(p_x, p_y, dx, dy, target.x, target.y, target.radius);
		if (t >= 0.0f && t < best_t) {
			best_t = t;
			hit.target = target.id;
		}
	}
	hit.end_x = p_x + dx * best_t;
	hit.end_y = p_y + dy * best_t;
	return hit;
}

void explosion_victims(const MazeGrid &p_grid, float p_x, float p_y, float p_radius,
		const std::vector<ProjectileTarget> &p_targets, int p_owner, std::vector<int> &r_out) {
	for (const ProjectileTarget &target : p_targets) {
		if (target.id == p_owner) {
			continue;
		}
		const float dx = target.x - p_x;
		const float dy = target.y - p_y;
		const float reach = p_radius + target.radius;
		if (dx * dx + dy * dy > reach * reach) {
			continue;
		}
		if (has_line_of_sight(p_grid, { p_x, p_y }, { target.x, target.y })) {
			r_out.push_back(target.id);
		}
	}
}

} // namespace lv::core
