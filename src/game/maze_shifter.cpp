#include "game/maze_shifter.h"

#include "core/perf.h"

#include "game/maze.h"
#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>

void MazeShifter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start", "maze", "seed"), &MazeShifter::start);
	ClassDB::bind_method(D_METHOD("stop"), &MazeShifter::stop);
	ClassDB::bind_method(D_METHOD("shift_now"), &MazeShifter::shift_now);
	ClassDB::bind_method(D_METHOD("get_state"), &MazeShifter::get_state);
	ClassDB::bind_method(D_METHOD("set_enabled", "value"), &MazeShifter::set_enabled);
	ClassDB::bind_method(D_METHOD("is_enabled"), &MazeShifter::is_enabled);
	ClassDB::bind_method(D_METHOD("set_interval", "value"), &MazeShifter::set_interval);
	ClassDB::bind_method(D_METHOD("get_interval"), &MazeShifter::get_interval);
	ClassDB::bind_method(D_METHOD("set_pairs", "value"), &MazeShifter::set_pairs);
	ClassDB::bind_method(D_METHOD("get_pairs"), &MazeShifter::get_pairs);
	ClassDB::bind_method(D_METHOD("set_warning_time", "value"), &MazeShifter::set_warning_time);
	ClassDB::bind_method(D_METHOD("get_warning_time"), &MazeShifter::get_warning_time);
	ClassDB::bind_method(D_METHOD("set_max_block_wait", "value"), &MazeShifter::set_max_block_wait);
	ClassDB::bind_method(D_METHOD("get_max_block_wait"), &MazeShifter::get_max_block_wait);
	ClassDB::bind_method(D_METHOD("set_block_margin", "value"), &MazeShifter::set_block_margin);
	ClassDB::bind_method(D_METHOD("get_block_margin"), &MazeShifter::get_block_margin);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "interval", PROPERTY_HINT_RANGE, "2,120,0.5,suffix:s"), "set_interval", "get_interval");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "pairs", PROPERTY_HINT_RANGE, "1,20,1"), "set_pairs", "get_pairs");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "warning_time", PROPERTY_HINT_RANGE, "0.5,10,0.1,suffix:s"), "set_warning_time", "get_warning_time");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_block_wait", PROPERTY_HINT_RANGE, "0,15,0.5,suffix:s"), "set_max_block_wait", "get_max_block_wait");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "block_margin", PROPERTY_HINT_RANGE, "0,64,1,suffix:px"), "set_block_margin", "get_block_margin");

	BIND_ENUM_CONSTANT(STATE_IDLE);
	BIND_ENUM_CONSTANT(STATE_WARNING);
	BIND_ENUM_CONSTANT(STATE_APPLYING);

	// closing/opening: Array de Vector2i (casillas). Los avisos se muestran durante `seconds`.
	ADD_SIGNAL(MethodInfo("shift_warned", PropertyInfo(Variant::ARRAY, "closing"), PropertyInfo(Variant::ARRAY, "opening"), PropertyInfo(Variant::FLOAT, "seconds")));
	ADD_SIGNAL(MethodInfo("closure_blocked", PropertyInfo(Variant::ARRAY, "tiles")));
	ADD_SIGNAL(MethodInfo("closure_cancelled", PropertyInfo(Variant::ARRAY, "tiles")));
	ADD_SIGNAL(MethodInfo("shift_finished"));
}

void MazeShifter::start(Maze *p_maze, int p_seed) {
	if (maze_ != nullptr && maze_->is_connected("map_changed", callable_mp(this, &MazeShifter::on_map_changed))) {
		maze_->disconnect("map_changed", callable_mp(this, &MazeShifter::on_map_changed));
	}
	maze_ = p_maze;
	// Red de seguridad: cuando un cierre se aplica de verdad (puede ser unos frames después, por la cola).
	maze_->connect("map_changed", callable_mp(this, &MazeShifter::on_map_changed));
	rng_ = std::make_unique<lv::core::Rng>(static_cast<uint32_t>(p_seed) ^ 0x51ED270Bu);
	planned_.clear();
	closures_.clear();
	state_ = STATE_IDLE;
	timer_ = interval_;
}

void MazeShifter::stop() {
	maze_ = nullptr;
	planned_.clear();
	closures_.clear();
	state_ = STATE_IDLE;
}

void MazeShifter::shift_now() {
	if (maze_ != nullptr && state_ == STATE_IDLE) {
		begin_warning();
	}
}

void MazeShifter::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint() || maze_ == nullptr) {
		return;
	}
	lv::core::PerfScope perf("maze.shifter");
	switch (state_) {
		case STATE_IDLE:
			if (!enabled_) {
				return;
			}
			timer_ -= static_cast<float>(p_delta);
			if (timer_ <= 0.0f) {
				begin_warning();
			}
			break;
		case STATE_WARNING:
			timer_ -= static_cast<float>(p_delta);
			if (timer_ <= 0.0f) {
				begin_applying();
			}
			break;
		case STATE_APPLYING:
			update_closures(p_delta);
			break;
	}
}

void MazeShifter::begin_warning() {
	planned_ = lv::core::plan_maze_shift(maze_->get_grid(), pairs_, *rng_);
	if (planned_.empty()) {
		timer_ = interval_;
		return;
	}
	TypedArray<Vector2i> closing;
	TypedArray<Vector2i> opening;
	for (const lv::core::PassageChange &change : planned_) {
		(change.open ? opening : closing).append_array(tiles_of(change));
	}
	state_ = STATE_WARNING;
	timer_ = warning_time_;
	emit_signal("shift_warned", closing, opening, warning_time_);
}

void MazeShifter::begin_applying() {
	closures_.clear();
	for (const lv::core::PassageChange &change : planned_) {
		if (change.open) {
			maze_->queue_passage_change(change.a, change.b, true);
		} else {
			closures_.push_back({ change, 0.0f });
		}
	}
	planned_.clear();
	state_ = STATE_APPLYING;
	update_closures(0.0);
}

void MazeShifter::update_closures(double p_delta) {
	for (auto it = closures_.begin(); it != closures_.end();) {
		if (!is_occupied(it->change)) {
			maze_->queue_passage_change(it->change.a, it->change.b, false);
			it = closures_.erase(it);
			continue;
		}
		if (it->waited == 0.0f) {
			emit_signal("closure_blocked", tiles_of(it->change));
		}
		it->waited += static_cast<float>(p_delta);
		if (it->waited >= max_block_wait_) {
			emit_signal("closure_cancelled", tiles_of(it->change)); // nunca se aplasta a nadie
			it = closures_.erase(it);
			continue;
		}
		++it;
	}
	if (closures_.empty()) {
		state_ = STATE_IDLE;
		timer_ = interval_;
		emit_signal("shift_finished");
	}
}

bool MazeShifter::is_occupied(const lv::core::PassageChange &p_change) const {
	const float half = maze_->get_tile_px() / 2.0f + block_margin_;
	const TypedArray<Node> participants = get_tree()->get_nodes_in_group("participants");
	for (const lv::core::Cell &tile : maze_->get_grid().passage_tiles(p_change.a, p_change.b)) {
		const Vector2 center = maze_->cell_to_world(Vector2i(tile.x, tile.y));
		for (int i = 0; i < participants.size(); i++) {
			const Participant *p = Object::cast_to<Participant>(participants[i]);
			if (p == nullptr) {
				continue;
			}
			const Vector2 d = (p->get_global_position() - center).abs();
			if (d.x <= half && d.y <= half) {
				return true;
			}
		}
	}
	return false;
}

void MazeShifter::rescue_trapped_participants() {
	const TypedArray<Node> participants = get_tree()->get_nodes_in_group("participants");
	for (int i = 0; i < participants.size(); i++) {
		Participant *p = Object::cast_to<Participant>(participants[i]);
		if (p != nullptr && maze_->is_wall(maze_->world_to_cell(p->get_global_position()))) {
			p->set_global_position(maze_->nearest_floor_world(p->get_global_position()));
		}
	}
}

TypedArray<Vector2i> MazeShifter::tiles_of(const lv::core::PassageChange &p_change) const {
	TypedArray<Vector2i> tiles;
	for (const lv::core::Cell &tile : maze_->get_grid().passage_tiles(p_change.a, p_change.b)) {
		tiles.push_back(Vector2i(tile.x, tile.y));
	}
	return tiles;
}

void MazeShifter::on_map_changed(const TypedArray<Vector2i> &, bool p_now_wall) {
	if (p_now_wall) {
		rescue_trapped_participants();
	}
}