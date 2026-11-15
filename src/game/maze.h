#pragma once

#include "core/line_of_sight.h"
#include "core/maze_grid.h"
#include "game/maze_profile.h"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/tile_map_layer.hpp>
#include <godot_cpp/variant/dictionary.hpp>
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
	// Máscara de luz de la capa de paredes: la luz de visión las ilumina sin sombrearlas.
	static constexpr int WALLS_LIGHT_MASK = 2;

	void _ready() override;

	void generate();

	bool is_wall(const Vector2i &p_cell) const;
	Vector2i get_center_cell() const;
	TypedArray<Vector2i> get_base_cells(int p_count) const;
	Vector2i get_grid_size() const;
	Vector2 cell_to_world(const Vector2i &p_cell) const;
	Vector2i world_to_cell(const Vector2 &p_world) const;
	float get_tile_px() const;

	// Visión (lógica en core/line_of_sight): ¿se ve `p_to` desde `p_from`? Posiciones globales;
	// radio en casillas. Lo usarán los bots para reaccionar solo a lo que realmente ven.
	bool has_line_of_sight(const Vector2 &p_from, const Vector2 &p_to) const;
	bool can_see(const Vector2 &p_from, const Vector2 &p_to, float p_radius_tiles) const;

	const lv::core::MazeGrid &get_grid() const { return grid_; }
	// Posición global -> coordenadas de casilla con decimales (para la lógica de core).
	lv::core::GridPoint world_to_grid_point(const Vector2 &p_world) const;

	// Propiedades editables desde el inspector.
	void set_cells_width(int p_value) { options_.cells_w = p_value; }
	int get_cells_width() const { return options_.cells_w; }
	void set_cells_height(int p_value) { options_.cells_h = p_value; }
	int get_cells_height() const { return options_.cells_h; }
	void set_corridor_width(int p_value) { options_.corridor_width = p_value; }
	int get_corridor_width() const { return options_.corridor_width; }
	void set_seed(int p_value) { seed_ = p_value; }
	int get_seed() const { return seed_; }
	void set_loop_chance(float p_value) { options_.loop_chance = p_value; }
	float get_loop_chance() const { return options_.loop_chance; }
	void set_vault_radius(int p_value) { options_.vault_radius = p_value; }
	int get_vault_radius() const { return options_.vault_radius; }
	void set_generate_on_ready(bool p_value) { generate_on_ready_ = p_value; }
	bool get_generate_on_ready() const { return generate_on_ready_; }
	int get_last_seed() const { return static_cast<int>(last_seed_); }

	// Perfil de generación (nivel o personalizado). Si hay perfil, manda sobre las propiedades sueltas.
	void set_profile(const Ref<MazeProfile> &p_value) { profile_ = p_value; }
	Ref<MazeProfile> get_profile() const { return profile_; }
	// {algorithm, cells, dead_ends, junctions, dead_end_ratio, route_length} del último laberinto.
	Dictionary get_metrics() const;

	// --- Mapa cambiante (§15) ---
	// Abre o cierra el pasaje entre dos celdas lógicas: actualiza la lógica, las casillas (colisión
	// y sombras) y emite `map_changed(tiles, now_wall)` (Observer) para que el resto se adapte.
	void set_passage(int p_a, int p_b, bool p_open);
	// Centro (mundo) de la casilla de suelo más cercana a una posición.
	Vector2 nearest_floor_world(const Vector2 &p_world) const;

	// Vista previa sin crear un nivel (para el menú de configuración): genera el laberinto del perfil
	// con esa semilla (0 = aleatoria) y devuelve {image: Image (1 px por casilla), metrics, seed, size}.
	static Dictionary build_preview(const Ref<MazeProfile> &p_profile, int p_seed);

protected:
	static void _bind_methods();

private:
	lv::core::MazeGrid grid_;
	Ref<MazeProfile> profile_;
	lv::core::MazeGrid::Options options_ = default_options();

	static lv::core::MazeGrid::Options default_options() {
		lv::core::MazeGrid::Options options;
		options.cells_w = 20;
		options.cells_h = 14;
		options.corridor_width = 3;
		options.vault_radius = 4;
		return options;
	}
	int seed_ = 0; // 0 = aleatoria en cada generación
	uint32_t last_seed_ = 0;
	bool generate_on_ready_ = true;

	TileMapLayer *walls_ = nullptr; // capa hija con las paredes (colisión + sombras)

	void ensure_tile_set();
	void ensure_walls_layer();
};
