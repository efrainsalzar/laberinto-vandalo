#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace lv::core {

// Índice espacial por cubetas (spatial hash): responde "¿quién está cerca de este punto?" sin recorrer
// todas las entidades. El mundo se divide en cubetas cuadradas de `cell_size`; cada entidad vive en una.
// Insertar/mover/quitar: O(1). Consulta de radio r: revisa solo las cubetas que toca el círculo.
// Lógica pura (ids enteros). Ver docs/ARQUITECTURA.md §17 y RENDIMIENTO.md (regla 7).
class SpatialHash {
public:
	explicit SpatialHash(float p_cell_size = 64.0f) :
			cell_size_(p_cell_size > 1.0f ? p_cell_size : 1.0f) {}

	void insert(int p_id, float p_x, float p_y);
	// Actualiza la posición. Si sigue en la misma cubeta, casi no cuesta nada.
	void move(int p_id, float p_x, float p_y);
	void remove(int p_id);
	void clear();

	// Ids dentro del círculo (centro, radio). Añade al final de r_out.
	void query_radius(float p_x, float p_y, float p_radius, std::vector<int> &r_out) const;
	int size() const { return static_cast<int>(entries_.size()); }

private:
	struct Entry {
		float x;
		float y;
		int64_t bucket;
	};

	float cell_size_;
	std::unordered_map<int, Entry> entries_;
	std::unordered_map<int64_t, std::vector<int>> buckets_;

	int64_t bucket_of(float p_x, float p_y) const;
	static int64_t pack(int p_bx, int p_by) { return (static_cast<int64_t>(p_bx) << 32) ^ static_cast<uint32_t>(p_by); }
	void bucket_remove(int64_t p_bucket, int p_id);
};

} // namespace lv::core
