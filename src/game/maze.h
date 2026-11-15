#pragma once

#include "core/maze_grid.h"

#include <godot_cpp/classes/tile_map_layer.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

using namespace godot;

// Nodo de Godot que dibuja un lv::core::MazeGrid con un TileMapLayer.
// Es un adaptador: la lógica del laberinto vive en core/; aquí solo se pinta y se traducen coordenadas.
// Ver docs/ARQUITECTURA.md §4.
class Maze : public TileMapLayer {
	GDCLASS(Maze, TileMapLayer)

public:
	// Casillas del atlas en el TileSet (fila 0: pared, suelo).
	static constexpr int SOURCE_ID = 0;
	static inline const Vector2i WALL_TILE = Vector2i(0, 0);
	static inline const Vector2i FLOOR_TILE = Vector2i(1, 0);

	void _ready() override;

	void generate();

	bool is_wall(const Vector2i &p_cell) const;
	Vector2i get_center_cell() const;
	TypedArray<Vector2i> get_base_cells(int p_count) const;
	Vector2i get_grid_size() const;
	Vector2 cell_to_world(const Vector2i &p_cell) const;
	Vector2i world_to_cell(const Vector2 &p_world) const;

	const lv::core::MazeGrid &get_grid() const { return grid_; }

	// Propiedades editables desde el inspector.
	void set_cells_width(int p_value) { options_.cells_w = p_value; }
	int get_cells_width() const { return options_.cells_w; }
	void set_cells_height(int p_value) { options_.cells_h = p_value; }
	int get_cells_height() const { return options_.cells_h; }
	void set_seed(int p_value) { seed_ = p_value; }
	int get_seed() const { return seed_; }
	void set_loop_chance(float p_value) { options_.loop_chance = p_value; }
	float get_loop_chance() const { return options_.loop_chance; }
	void set_vault_radius(int p_value) { options_.vault_radius = p_value; }
	int get_vault_radius() const { return options_.vault_radius; }
	void set_generate_on_ready(bool p_value) { generate_on_ready_ = p_value; }
	bool get_generate_on_ready() const { return generate_on_ready_; }
	int get_last_seed() const { return static_cast<int>(last_seed_); }

protected:
	static void _bind_methods();

private:
	lv::core::MazeGrid grid_;
	lv::core::MazeGrid::Options options_;
	int seed_ = 0; // 0 = aleatoria en cada generación
	uint32_t last_seed_ = 0;
	bool generate_on_ready_ = true;

	void ensure_tile_set();
};
