#include "core/map_change_queue.h"

#include <algorithm>

namespace lv::core {

int64_t MapChangeQueue::key(int p_a, int p_b) {
	if (p_a > p_b) {
		std::swap(p_a, p_b);
	}
	return (static_cast<int64_t>(p_a) << 32) | static_cast<uint32_t>(p_b);
}

bool MapChangeQueue::blocked_by_reservation(const MapChangeOrder &p_order) const {
	const auto it = reservations_.find(key(p_order.change.a, p_order.change.b));
	if (it == reservations_.end() || it->second.source == p_order.source) {
		return false;
	}
	return static_cast<int>(p_order.priority) <= static_cast<int>(it->second.priority);
}

bool MapChangeQueue::push(const MapChangeOrder &p_order) {
	if (blocked_by_reservation(p_order)) {
		dropped_++;
		return false;
	}
	const int64_t k = key(p_order.change.a, p_order.change.b);
	// Una sola orden por pasaje: gana la más prioritaria (en empate, la que ya estaba).
	for (auto it = orders_.begin(); it != orders_.end(); ++it) {
		if (key(it->change.a, it->change.b) != k) {
			continue;
		}
		if (static_cast<int>(p_order.priority) <= static_cast<int>(it->priority)) {
			dropped_++;
			return false;
		}
		orders_.erase(it);
		dropped_++;
		break;
	}
	// Una orden más prioritaria pisa la reserva de otra fuente.
	const auto res = reservations_.find(k);
	if (res != reservations_.end() && res->second.source != p_order.source) {
		reservations_.erase(res);
	}
	orders_.push_back(p_order);
	// Tope: descartar la de menor prioridad (la más nueva en empate).
	while (static_cast<int>(orders_.size()) > capacity_) {
		auto worst = orders_.begin();
		for (auto it = orders_.begin(); it != orders_.end(); ++it) {
			if (static_cast<int>(it->priority) <= static_cast<int>(worst->priority)) {
				worst = it;
			}
		}
		const bool rejected_new = (worst == orders_.end() - 1);
		orders_.erase(worst);
		dropped_++;
		if (rejected_new) {
			return false;
		}
	}
	return true;
}

std::vector<MapChangeOrder> MapChangeQueue::take(int p_max) {
	std::vector<MapChangeOrder> taken;
	while (static_cast<int>(taken.size()) < p_max && !orders_.empty()) {
		auto best = orders_.begin();
		for (auto it = orders_.begin(); it != orders_.end(); ++it) {
			if (static_cast<int>(it->priority) > static_cast<int>(best->priority)) {
				best = it; // la primera con la prioridad más alta: FIFO dentro de la misma prioridad
			}
		}
		taken.push_back(*best);
		orders_.erase(best);
	}
	return taken;
}

bool MapChangeQueue::reserve(int p_a, int p_b, int p_source, ChangePriority p_priority) {
	const int64_t k = key(p_a, p_b);
	const auto it = reservations_.find(k);
	if (it != reservations_.end() && it->second.source != p_source &&
			static_cast<int>(p_priority) <= static_cast<int>(it->second.priority)) {
		return false;
	}
	reservations_[k] = { p_source, p_priority };
	return true;
}

void MapChangeQueue::release(int p_a, int p_b, int p_source) {
	const auto it = reservations_.find(key(p_a, p_b));
	if (it != reservations_.end() && it->second.source == p_source) {
		reservations_.erase(it);
	}
}

bool MapChangeQueue::is_reserved_by(int p_a, int p_b, int p_source) const {
	const auto it = reservations_.find(key(p_a, p_b));
	return it != reservations_.end() && it->second.source == p_source;
}

void MapChangeQueue::clear() {
	orders_.clear();
	reservations_.clear();
	dropped_ = 0;
}

} // namespace lv::core
