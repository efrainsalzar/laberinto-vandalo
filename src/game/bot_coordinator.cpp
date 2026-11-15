#include "game/bot_coordinator.h"

#include "core/nav_path.h"
#include "core/perf.h"
#include "game/bot_controller.h"
#include "game/maze.h"
#include "game/participant.h"
#include "game/pickup_factory.h"
#include "game/trophy_controller.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>

void BotCoordinator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "maze", "trophy", "pickups"), &BotCoordinator::setup);
	ClassDB::bind_method(D_METHOD("add_bot", "participant", "profile", "base_zone", "seed"), &BotCoordinator::add_bot);
	ClassDB::bind_method(D_METHOD("clear"), &BotCoordinator::clear);
	ClassDB::bind_method(D_METHOD("get_bot_count"), &BotCoordinator::get_bot_count);
	ClassDB::bind_method(D_METHOD("set_max_paths_per_frame", "value"), &BotCoordinator::set_max_paths_per_frame);
	ClassDB::bind_method(D_METHOD("get_max_paths_per_frame"), &BotCoordinator::get_max_paths_per_frame);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_paths_per_frame", PROPERTY_HINT_RANGE, "1,16,1"), "set_max_paths_per_frame", "get_max_paths_per_frame");
	BIND_CONSTANT(MAX_BOTS);
}

void BotCoordinator::setup(Maze *p_maze, TrophyController *p_trophy, PickupFactory *p_pickups) {
	clear();
	const Callable on_changed = callable_mp(this, &BotCoordinator::on_map_changed);
	if (maze_ != nullptr && maze_->is_connected("map_changed", on_changed)) {
		maze_->disconnect("map_changed", on_changed);
	}
	maze_ = p_maze;
	trophy_ = p_trophy;
	pickups_ = p_pickups;
	if (maze_ != nullptr) {
		maze_->connect("map_changed", on_changed);
	}
}

Ref<BotController> BotCoordinator::add_bot(Participant *p_participant, const Ref<BotProfile> &p_profile, const Rect2i &p_base_zone, int p_seed) {
	if (p_participant == nullptr || maze_ == nullptr) {
		UtilityFunctions::push_error("BotCoordinator.add_bot: falta el participante o setup().");
		return Ref<BotController>();
	}
	if (static_cast<int>(bots_.size()) >= MAX_BOTS) {
		UtilityFunctions::push_warning("BotCoordinator: máximo ", MAX_BOTS, " bots; se ignora el resto.");
		return Ref<BotController>();
	}
	Ref<BotController> bot;
	bot.instantiate();
	bot->setup(this, p_profile, p_base_zone, static_cast<uint32_t>(p_seed));
	bots_.push_back(bot);
	bot_bodies_.push_back(p_participant);
	p_participant->set_controller(bot);
	return bot;
}

void BotCoordinator::clear() {
	for (const Ref<BotController> &bot : bots_) {
		bot->detach();
	}
	bots_.clear();
	bot_bodies_.clear();
	requests_.clear();
	chase_tokens_.clear();
	ambush_cache_.clear();
}

void BotCoordinator::request_path(const Ref<BotController> &p_bot, const Vector2i &p_from_tile, const Vector2i &p_to_tile) {
	requests_.push_back({ p_bot, p_from_tile, p_to_tile, p_bot.is_valid() ? p_bot->get_generation() : 0 });
}

void BotCoordinator::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint() || maze_ == nullptr) {
		return;
	}
	token_timer_ -= static_cast<float>(p_delta);
	if (token_timer_ <= 0.0f) {
		token_timer_ = TOKEN_INTERVAL;
		update_chase_tokens();
	}
	if (requests_.empty()) {
		return;
	}
	lv::core::PerfScope perf("bots.path");
	const lv::core::MazeGrid &grid = maze_->get_grid();
	// Presupuesto: como máximo N caminos por frame; el resto espera su turno (FIFO).
	for (int i = 0; i < max_paths_per_frame_ && !requests_.empty(); i++) {
		const PathRequest request = requests_.front();
		requests_.pop_front();
		if (request.bot.is_null()) {
			continue;
		}
		const int from = grid.cell_index_of({ request.from.x, request.from.y });
		const int to = grid.cell_index_of({ request.to.x, request.to.y });
		request.bot->apply_path(lv::core::find_path(grid.passages(), from, to), maze_->cell_to_world(request.to), request.generation);
	}
}

void BotCoordinator::on_map_changed(const TypedArray<Vector2i> &, bool p_now_wall) {
	ambush_cache_.clear(); // los caminos a las bases pueden haber cambiado
	if (!p_now_wall) {
		return; // abrir pasajes no rompe ningún camino (a lo sumo deja de ser el más corto)
	}
	for (const Ref<BotController> &bot : bots_) {
		bot->validate_path();
	}
}

void BotCoordinator::build_world(const Participant &p_participant, lv::core::BotWorld &r_world) const {
	const Vector2 position = p_participant.get_global_position();
	const Vector2i tile = maze_->world_to_cell(position);
	r_world.self = { tile.x, tile.y };
	r_world.seen_pickups.clear();

	if (trophy_ != nullptr) {
		const Participant *carrier = trophy_->get_carrier();
		r_world.carrying_trophy = carrier == &p_participant;
		r_world.trophy_available = !trophy_->is_finished() && carrier == nullptr;
		r_world.trophy_carried_by_other = !trophy_->is_finished() && carrier != nullptr && carrier != &p_participant;
		r_world.may_chase = r_world.trophy_carried_by_other && chase_tokens_.has(bot_index(&p_participant));
		r_world.has_ambush = r_world.trophy_carried_by_other && ambush_for(carrier, r_world.ambush);
		const Vector2i trophy_tile = maze_->world_to_cell(trophy_->get_trophy_position());
		r_world.trophy = { trophy_tile.x, trophy_tile.y };
	}

	// Objetos que VE: dentro de su radio de visión y con línea de visión (juego limpio).
	if (pickups_ != nullptr) {
		const StatsComponent *stats = p_participant.get_stats();
		const float radius = stats != nullptr ? stats->get_vision() : 5.0f;
		const int r = static_cast<int>(std::ceil(radius));
		for (int dy = -r; dy <= r && static_cast<int>(r_world.seen_pickups.size()) < MAX_SEEN_PICKUPS; dy++) {
			for (int dx = -r; dx <= r; dx++) {
				const Vector2i t(tile.x + dx, tile.y + dy);
				if (!pickups_->has_pickup_at(t)) {
					continue;
				}
				if (maze_->can_see(position, maze_->cell_to_world(t), radius)) {
					r_world.seen_pickups.push_back({ t.x, t.y });
					if (static_cast<int>(r_world.seen_pickups.size()) >= MAX_SEEN_PICKUPS) {
						break;
					}
				}
			}
		}
	}
}

int BotCoordinator::bot_index(const Participant *p_participant) const {
	for (size_t i = 0; i < bot_bodies_.size(); i++) {
		if (bot_bodies_[i] == p_participant) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

// Fichas de ataque: candidatos = bots que no llevan el trofeo, por distancia al portador.
void BotCoordinator::update_chase_tokens() {
	lv::core::PerfScope perf("bots.tokens");
	const Participant *carrier = trophy_ != nullptr ? trophy_->get_carrier() : nullptr;
	if (carrier == nullptr) {
		chase_tokens_.clear();
		return;
	}
	chase_tokens_.set_max(trophy_->rules().max_chasers);
	const Vector2 at = carrier->get_global_position();
	std::vector<lv::core::TokenPool::Candidate> candidates;
	for (size_t i = 0; i < bot_bodies_.size(); i++) {
		if (bot_bodies_[i] != nullptr && bot_bodies_[i] != carrier && bot_bodies_[i]->is_active()) {
			candidates.push_back({ static_cast<int>(i), bot_bodies_[i]->get_global_position().distance_to(at) });
		}
	}
	chase_tokens_.assign(candidates);
}

// Punto de emboscada: AMBUSH_CELLS_FROM_BASE celdas desde la base del portador hacia el centro (por donde
// tiene que pasar). Fuera de la zona: así se lo pueden quitar antes de que entregue.
bool BotCoordinator::ambush_for(const Participant *p_carrier, lv::core::Cell &r_cell) const {
	if (p_carrier == nullptr || trophy_ == nullptr) {
		return false;
	}
	const auto cached = ambush_cache_.find(p_carrier);
	if (cached != ambush_cache_.end()) {
		r_cell = cached->second;
		return true;
	}
	const lv::core::MazeGrid &grid = maze_->get_grid();
	const Vector2i base_tile = maze_->world_to_cell(trophy_->get_base_position(const_cast<Participant *>(p_carrier)));
	const int from = grid.cell_index_of({ base_tile.x, base_tile.y });
	const int to = grid.cell_index_of(grid.center());
	const std::vector<int> path = lv::core::find_path(grid.passages(), from, to);
	if (path.empty()) {
		return false;
	}
	const size_t index = std::min(path.size() - 1, static_cast<size_t>(AMBUSH_CELLS_FROM_BASE));
	r_cell = grid.cell_center(path[index]);
	ambush_cache_[p_carrier] = r_cell;
	return true;
}
