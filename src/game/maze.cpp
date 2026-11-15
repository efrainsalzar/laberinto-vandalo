#include "game/maze.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/classes/tile_data.hpp>
#include <godot_cpp/classes/tile_set.hpp>
#include <godot_cpp/classes/tile_set_atlas_source.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace {
constexpr int TILE_PX = 16;

lv::core::Cell to_cell(const Vector2i &p_v) {
	return { p_v.x, p_v.y };
}

Vector2i to_vector(const lv::core::Cell &p_c) {
	return Vector2i(p_c.x, p_c.y);
}
} // namespace

void Maze::_bind_methods() {
	ClassDB::bind_method(D_METHOD("generate"), &Maze::generate);
	ClassDB::bind_method(D_METHOD("is_wall", "cell"), &Maze::is_wall);
	ClassDB::bind_method(D_METHOD("get_center_cell"), &Maze::get_center_cell);
	ClassDB::bind_method(D_METHOD("get_base_cells", "count"), &Maze::get_base_cells);
	ClassDB::bind_method(D_METHOD("get_grid_size"), &Maze::get_grid_size);
	ClassDB::bind_method(D_METHOD("cell_to_world", "cell"), &Maze::cell_to_world);
	ClassDB::bind_method(D_METHOD("world_to_cell", "world_position"), &Maze::world_to_cell);
	ClassDB::bind_method(D_METHOD("get_last_seed"), &Maze::get_last_seed);

	ClassDB::bind_method(D_METHOD("set_cells_width", "value"), &Maze::set_cells_width);
	ClassDB::bind_method(D_METHOD("get_cells_width"), &Maze::get_cells_width);
	ClassDB::bind_method(D_METHOD("set_cells_height", "value"), &Maze::set_cells_height);
	ClassDB::bind_method(D_METHOD("get_cells_height"), &Maze::get_cells_height);
	ClassDB::bind_method(D_METHOD("set_seed", "value"), &Maze::set_seed);
	ClassDB::bind_method(D_METHOD("get_seed"), &Maze::get_seed);
	ClassDB::bind_method(D_METHOD("set_loop_chance", "value"), &Maze::set_loop_chance);
	ClassDB::bind_method(D_METHOD("get_loop_chance"), &Maze::get_loop_chance);
	ClassDB::bind_method(D_METHOD("set_vault_radius", "value"), &Maze::set_vault_radius);
	ClassDB::bind_method(D_METHOD("get_vault_radius"), &Maze::get_vault_radius);
	ClassDB::bind_method(D_METHOD("set_generate_on_ready", "value"), &Maze::set_generate_on_ready);
	ClassDB::bind_method(D_METHOD("get_generate_on_ready"), &Maze::get_generate_on_ready);

	ADD_GROUP("Generación", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cells_width", PROPERTY_HINT_RANGE, "2,100,1"), "set_cells_width", "get_cells_width");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cells_height", PROPERTY_HINT_RANGE, "2,100,1"), "set_cells_height", "get_cells_height");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "loop_chance", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_loop_chance", "get_loop_chance");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "vault_radius", PROPERTY_HINT_RANGE, "0,10,1"), "set_vault_radius", "get_vault_radius");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "generate_on_ready"), "set_generate_on_ready", "get_generate_on_ready");

	ADD_SIGNAL(MethodInfo("generated", PropertyInfo(Variant::INT, "seed")));
}

void Maze::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (generate_on_ready_) {
		generate();
	}
}

void Maze::generate() {
	if (seed_ != 0) {
		last_seed_ = static_cast<uint32_t>(seed_);
	} else {
		Ref<RandomNumberGenerator> rng;
		rng.instantiate();
		rng->randomize();
		last_seed_ = rng->randi_range(1, 2147483647);
	}

	lv::core::MazeGrid::Options options = options_;
	options.seed = last_seed_;
	grid_.generate(options);

	ensure_tile_set();
	clear();
	for (int y = 0; y < grid_.height(); y++) {
		for (int x = 0; x < grid_.width(); x++) {
			const bool wall = grid_.is_wall({ x, y });
			set_cell(Vector2i(x, y), SOURCE_ID, wall ? WALL_TILE : FLOOR_TILE);
		}
	}
	emit_signal("generated", static_cast<int>(last_seed_));
}

// TileSet provisional (dos colores, colisión en las paredes) para no depender de arte todavía.
// Cuando haya arte, se asigna un TileSet en el editor con las mismas casillas (WALL_TILE, FLOOR_TILE).
void Maze::ensure_tile_set() {
	if (get_tile_set().is_valid()) {
		return;
	}
	Ref<Image> image = Image::create_empty(TILE_PX * 2, TILE_PX, false, Image::FORMAT_RGBA8);
	image->fill_rect(Rect2i(0, 0, TILE_PX, TILE_PX), Color(0.20f, 0.18f, 0.28f)); // pared
	image->fill_rect(Rect2i(1, 1, TILE_PX - 2, TILE_PX - 2), Color(0.27f, 0.24f, 0.37f));
	image->fill_rect(Rect2i(TILE_PX, 0, TILE_PX, TILE_PX), Color(0.09f, 0.08f, 0.12f)); // suelo

	Ref<TileSet> tile_set;
	tile_set.instantiate();
	tile_set->set_tile_size(Vector2i(TILE_PX, TILE_PX));
	tile_set->add_physics_layer();

	Ref<TileSetAtlasSource> source;
	source.instantiate();
	source->set_texture(ImageTexture::create_from_image(image));
	source->set_texture_region_size(Vector2i(TILE_PX, TILE_PX));
	tile_set->add_source(source, SOURCE_ID);
	source->create_tile(WALL_TILE);
	source->create_tile(FLOOR_TILE);

	const float h = TILE_PX / 2.0f;
	PackedVector2Array square;
	square.push_back(Vector2(-h, -h));
	square.push_back(Vector2(h, -h));
	square.push_back(Vector2(h, h));
	square.push_back(Vector2(-h, h));
	TileData *wall_data = source->get_tile_data(WALL_TILE, 0);
	wall_data->add_collision_polygon(0);
	wall_data->set_collision_polygon_points(0, 0, square);

	set_tile_set(tile_set);
}

bool Maze::is_wall(const Vector2i &p_cell) const {
	return grid_.is_wall(to_cell(p_cell));
}

Vector2i Maze::get_center_cell() const {
	return to_vector(grid_.center());
}

TypedArray<Vector2i> Maze::get_base_cells(int p_count) const {
	TypedArray<Vector2i> result;
	for (const lv::core::Cell &c : grid_.base_cells(p_count)) {
		result.push_back(to_vector(c));
	}
	return result;
}

Vector2i Maze::get_grid_size() const {
	return Vector2i(grid_.width(), grid_.height());
}

Vector2 Maze::cell_to_world(const Vector2i &p_cell) const {
	return to_global(map_to_local(p_cell));
}

Vector2i Maze::world_to_cell(const Vector2 &p_world) const {
	return local_to_map(to_local(p_world));
}
