#include "game/maze.h"

#include "core/line_of_sight.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/occluder_polygon2d.hpp>
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
	BIND_CONSTANT(WALLS_LIGHT_MASK);
	ClassDB::bind_method(D_METHOD("generate"), &Maze::generate);
	ClassDB::bind_method(D_METHOD("is_wall", "cell"), &Maze::is_wall);
	ClassDB::bind_method(D_METHOD("get_center_cell"), &Maze::get_center_cell);
	ClassDB::bind_method(D_METHOD("get_base_cells", "count"), &Maze::get_base_cells);
	ClassDB::bind_method(D_METHOD("get_grid_size"), &Maze::get_grid_size);
	ClassDB::bind_method(D_METHOD("cell_to_world", "cell"), &Maze::cell_to_world);
	ClassDB::bind_method(D_METHOD("world_to_cell", "world_position"), &Maze::world_to_cell);
	ClassDB::bind_method(D_METHOD("get_last_seed"), &Maze::get_last_seed);
	ClassDB::bind_method(D_METHOD("get_tile_px"), &Maze::get_tile_px);
	ClassDB::bind_method(D_METHOD("set_profile", "value"), &Maze::set_profile);
	ClassDB::bind_method(D_METHOD("get_profile"), &Maze::get_profile);
	ClassDB::bind_method(D_METHOD("get_metrics"), &Maze::get_metrics);
	ClassDB::bind_method(D_METHOD("has_line_of_sight", "from", "to"), &Maze::has_line_of_sight);
	ClassDB::bind_method(D_METHOD("can_see", "from", "to", "radius_tiles"), &Maze::can_see);

	ClassDB::bind_method(D_METHOD("set_cells_width", "value"), &Maze::set_cells_width);
	ClassDB::bind_method(D_METHOD("get_cells_width"), &Maze::get_cells_width);
	ClassDB::bind_method(D_METHOD("set_cells_height", "value"), &Maze::set_cells_height);
	ClassDB::bind_method(D_METHOD("get_cells_height"), &Maze::get_cells_height);
	ClassDB::bind_method(D_METHOD("set_corridor_width", "value"), &Maze::set_corridor_width);
	ClassDB::bind_method(D_METHOD("get_corridor_width"), &Maze::get_corridor_width);
	ClassDB::bind_method(D_METHOD("set_seed", "value"), &Maze::set_seed);
	ClassDB::bind_method(D_METHOD("get_seed"), &Maze::get_seed);
	ClassDB::bind_method(D_METHOD("set_loop_chance", "value"), &Maze::set_loop_chance);
	ClassDB::bind_method(D_METHOD("get_loop_chance"), &Maze::get_loop_chance);
	ClassDB::bind_method(D_METHOD("set_vault_radius", "value"), &Maze::set_vault_radius);
	ClassDB::bind_method(D_METHOD("get_vault_radius"), &Maze::get_vault_radius);
	ClassDB::bind_method(D_METHOD("set_generate_on_ready", "value"), &Maze::set_generate_on_ready);
	ClassDB::bind_method(D_METHOD("get_generate_on_ready"), &Maze::get_generate_on_ready);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "profile", PROPERTY_HINT_RESOURCE_TYPE, "MazeProfile"), "set_profile", "get_profile");
	ADD_GROUP("Generación (sin perfil)", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cells_width", PROPERTY_HINT_RANGE, "2,100,1"), "set_cells_width", "get_cells_width");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cells_height", PROPERTY_HINT_RANGE, "2,100,1"), "set_cells_height", "get_cells_height");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "corridor_width", PROPERTY_HINT_RANGE, "1,8,1"), "set_corridor_width", "get_corridor_width");
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

	lv::core::MazeGrid::Options options = profile_.is_valid() ? profile_->to_options() : options_;
	options.seed = last_seed_;
	grid_.generate(options);

	ensure_tile_set();
	ensure_walls_layer();
	clear();
	walls_->clear();
	for (int y = 0; y < grid_.height(); y++) {
		for (int x = 0; x < grid_.width(); x++) {
			// El suelo va debajo de todo; las paredes, en su propia capa (ver ensure_walls_layer).
			set_cell(Vector2i(x, y), SOURCE_ID, FLOOR_TILE);
			if (grid_.is_wall({ x, y })) {
				walls_->set_cell(Vector2i(x, y), SOURCE_ID, WALL_TILE);
			}
		}
	}
	emit_signal("generated", static_cast<int>(last_seed_));
}

// Las paredes se dibujan en una capa hija con su propia máscara de luz (WALLS_LIGHT_MASK).
// La luz de visión las ilumina pero no les aplica sombras: se ven las paredes cercanas
// y lo que hay detrás de ellas (suelo, objetos, otros participantes) queda oculto.
void Maze::ensure_walls_layer() {
	if (walls_ != nullptr) {
		return;
	}
	walls_ = memnew(TileMapLayer);
	walls_->set_name("Walls");
	walls_->set_tile_set(get_tile_set());
	walls_->set_light_mask(WALLS_LIGHT_MASK);
	add_child(walls_);
}

// TileSet provisional (dos colores, colisión en las paredes) para no depender de arte todavía.
// Cuando haya arte, se asigna un TileSet en el editor con las mismas casillas (WALL_TILE, FLOOR_TILE).
void Maze::ensure_tile_set() {
	if (get_tile_set().is_valid()) {
		return;
	}
	Ref<Image> image = Image::create_empty(TILE_PX * 2, TILE_PX, false, Image::FORMAT_RGBA8);
	image->fill_rect(Rect2i(0, 0, TILE_PX, TILE_PX), Color(0.30f, 0.26f, 0.42f)); // pared
	image->fill_rect(Rect2i(1, 1, TILE_PX - 2, TILE_PX - 2), Color(0.40f, 0.35f, 0.55f));
	image->fill_rect(Rect2i(TILE_PX, 0, TILE_PX, TILE_PX), Color(0.17f, 0.16f, 0.22f)); // suelo

	Ref<TileSet> tile_set;
	tile_set.instantiate();
	tile_set->set_tile_size(Vector2i(TILE_PX, TILE_PX));
	tile_set->add_physics_layer();
	tile_set->add_occlusion_layer(); // las paredes proyectan sombra (visión limitada)

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
	Ref<OccluderPolygon2D> occluder;
	occluder.instantiate();
	occluder->set_polygon(square);
	wall_data->set_occluder(0, occluder);

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

float Maze::get_tile_px() const {
	return get_tile_set().is_valid() ? static_cast<float>(get_tile_set()->get_tile_size().x) : static_cast<float>(TILE_PX);
}

lv::core::GridPoint Maze::world_to_grid_point(const Vector2 &p_world) const {
	const Vector2 local = to_local(p_world) / get_tile_px();
	return { local.x, local.y };
}

bool Maze::has_line_of_sight(const Vector2 &p_from, const Vector2 &p_to) const {
	return lv::core::has_line_of_sight(grid_, world_to_grid_point(p_from), world_to_grid_point(p_to));
}

bool Maze::can_see(const Vector2 &p_from, const Vector2 &p_to, float p_radius_tiles) const {
	return lv::core::can_see(grid_, world_to_grid_point(p_from), world_to_grid_point(p_to), p_radius_tiles);
}
Dictionary Maze::get_metrics() const {
	const lv::core::MazeGrid::Metrics &m = grid_.metrics();
	Dictionary d;
	d["algorithm"] = String::utf8(lv::core::maze_algorithm_name(m.algorithm));
	d["cells"] = m.cells;
	d["dead_ends"] = m.dead_ends;
	d["junctions"] = m.junctions;
	d["dead_end_ratio"] = m.dead_end_ratio;
	d["route_length"] = m.route_length;
	return d;
}