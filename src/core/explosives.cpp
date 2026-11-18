#include "core/explosives.h"

namespace lv::core {

void ExplosiveWorld::set_max(int p_max) {
	max_ = p_max < 1 ? 1 : p_max;
	while (static_cast<int>(items_.size()) > max_) {
		items_.erase(items_.begin()); // el más viejo está primero
	}
}

void ExplosiveWorld::place(float p_x, float p_y, const ExplosiveSpec &p_spec, int p_owner, int p_tag) {
	if (static_cast<int>(items_.size()) >= max_) {
		items_.erase(items_.begin()); // tope: desaparece el más viejo (el arreglo queda en orden de creación)
	}
	Explosive e;
	e.x = p_x;
	e.y = p_y;
	e.owner = p_owner;
	e.tag = p_tag;
	e.spec = p_spec;
	e.timer = p_spec.kind == ExplosiveKind::Bomb ? p_spec.fuse : p_spec.arm_time;
	e.seq = next_seq_++;
	items_.push_back(e);
}

void ExplosiveWorld::step(float p_delta, const std::vector<ProjectileTarget> &p_targets, std::vector<ExplosiveEvent> &r_events) {
	size_t keep = 0;
	for (size_t i = 0; i < items_.size(); i++) {
		Explosive &e = items_[i];
		e.age += p_delta;
		bool boom = false;
		int trigger = -1;
		if (e.spec.kind == ExplosiveKind::Bomb) {
			e.timer -= p_delta;
			boom = e.timer <= 0.0f;
		} else {
			const bool was_armed = e.is_armed();
			e.timer -= p_delta;
			if (was_armed) { // recién armada no explota en el mismo paso: se ve armarse
				for (const ProjectileTarget &t : p_targets) {
					if (t.id == e.owner) {
						continue;
					}
					const float dx = t.x - e.x;
					const float dy = t.y - e.y;
					const float reach = e.spec.trigger_radius + t.radius;
					if (dx * dx + dy * dy <= reach * reach) {
						boom = true;
						trigger = t.id;
						break;
					}
				}
			}
		}
		if (boom) {
			ExplosiveEvent event;
			event.x = e.x;
			event.y = e.y;
			event.owner = e.owner;
			event.tag = e.tag;
			event.explosion_radius = e.spec.explosion_radius;
			event.kind = e.spec.kind;
			event.trigger = trigger;
			r_events.push_back(event);
			continue; // no se conserva
		}
		if (keep != i) {
			items_[keep] = e; // compacta conservando el orden (el más viejo sigue primero)
		}
		keep++;
	}
	items_.resize(keep);
}

} // namespace lv::core
