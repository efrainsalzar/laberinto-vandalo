#include "core/spatial_hash.h"

#include <algorithm>
#include <cmath>

namespace lv::core {

int64_t SpatialHash::bucket_of(float p_x, float p_y) const {
	return pack(static_cast<int>(std::floor(p_x / cell_size_)), static_cast<int>(std::floor(p_y / cell_size_)));
}

void SpatialHash::bucket_remove(int64_t p_bucket, int p_id) {
	const auto it = buckets_.find(p_bucket);
	if (it == buckets_.end()) {
		return;
	}
	std::vector<int> &ids = it->second;
	const auto pos = std::find(ids.begin(), ids.end(), p_id);
	if (pos != ids.end()) {
		*pos = ids.back();
		ids.pop_back();
	}
	if (ids.empty()) {
		buckets_.erase(it);
	}
}

void SpatialHash::insert(int p_id, float p_x, float p_y) {
	if (entries_.count(p_id) != 0) {
		move(p_id, p_x, p_y);
		return;
	}
	const int64_t b = bucket_of(p_x, p_y);
	entries_[p_id] = { p_x, p_y, b };
	buckets_[b].push_back(p_id);
}

void SpatialHash::move(int p_id, float p_x, float p_y) {
	const auto it = entries_.find(p_id);
	if (it == entries_.end()) {
		insert(p_id, p_x, p_y);
		return;
	}
	Entry &e = it->second;
	e.x = p_x;
	e.y = p_y;
	const int64_t b = bucket_of(p_x, p_y);
	if (b != e.bucket) {
		bucket_remove(e.bucket, p_id);
		buckets_[b].push_back(p_id);
		e.bucket = b;
	}
}

void SpatialHash::remove(int p_id) {
	const auto it = entries_.find(p_id);
	if (it == entries_.end()) {
		return;
	}
	bucket_remove(it->second.bucket, p_id);
	entries_.erase(it);
}

void SpatialHash::clear() {
	entries_.clear();
	buckets_.clear();
}

void SpatialHash::query_radius(float p_x, float p_y, float p_radius, std::vector<int> &r_out) const {
	const int bx0 = static_cast<int>(std::floor((p_x - p_radius) / cell_size_));
	const int bx1 = static_cast<int>(std::floor((p_x + p_radius) / cell_size_));
	const int by0 = static_cast<int>(std::floor((p_y - p_radius) / cell_size_));
	const int by1 = static_cast<int>(std::floor((p_y + p_radius) / cell_size_));
	const float r2 = p_radius * p_radius;
	for (int by = by0; by <= by1; by++) {
		for (int bx = bx0; bx <= bx1; bx++) {
			const auto it = buckets_.find(pack(bx, by));
			if (it == buckets_.end()) {
				continue;
			}
			for (int id : it->second) {
				const Entry &e = entries_.at(id);
				const float dx = e.x - p_x;
				const float dy = e.y - p_y;
				if (dx * dx + dy * dy <= r2) {
					r_out.push_back(id);
				}
			}
		}
	}
}

} // namespace lv::core
