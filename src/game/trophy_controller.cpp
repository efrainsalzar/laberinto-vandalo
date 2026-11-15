#include "game/trophy_controller.h"

#include "core/perf.h"

#include "game/maze.h"
#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using lv::core::TrophyTracker;

void TrophyController::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start", "maze", "participants"), &TrophyController::start);
	ClassDB::bind_method(D_METHOD("drop_trophy"), &TrophyController::drop_trophy);
	ClassDB::bind_method(D_METHOD("get_carrier"), &TrophyController::get_carrier);
	ClassDB::bind_method(D_METHOD("get_winner"), &TrophyController::get_winner);
	ClassDB::bind_method(D_METHOD("is_finished"), &TrophyController::is_finished);
	ClassDB::bind_method(D_METHOD("get_trophy_position"), &TrophyController::get_trophy_position);
	ClassDB::bind_method(D_METHOD("get_base_position", "participant"), &TrophyController::get_base_position);
	ClassDB::bind_method(D_METHOD("set_trophy_scene", "value"), &TrophyController::set_trophy_scene);
	ClassDB::bind_method(D_METHOD("get_trophy_scene"), &TrophyController::get_trophy_scene);
	ClassDB::bind_method(D_METHOD("set_steal_enabled", "value"), &TrophyController::set_steal_enabled);
	ClassDB::bind_method(D_METHOD("is_steal_enabled"), &TrophyController::is_steal_enabled);
	ClassDB::bind_method(D_METHOD("set_steal_immunity", "value"), &TrophyController::set_steal_immunity);
	ClassDB::bind_method(D_METHOD("get_steal_immunity"), &TrophyController::get_steal_immunity);
	ClassDB::bind_method(D_METHOD("set_steal_margin", "value"), &TrophyController::set_steal_margin);
	ClassDB::bind_method(D_METHOD("get_steal_margin"), &TrophyController::get_steal_margin);
	ClassDB::bind_method(D_METHOD("get_immunity_left"), &TrophyController::get_immunity_left);
	ClassDB::bind_method(D_METHOD("set_base_scene", "value"), &TrophyController::set_base_scene);
	ClassDB::bind_method(D_METHOD("get_base_scene"), &TrophyController::get_base_scene);
	ClassDB::bind_method(D_METHOD("set_carrier_speed_multiplier", "value"), &TrophyController::set_carrier_speed_multiplier);
	ClassDB::bind_method(D_METHOD("get_carrier_speed_multiplier"), &TrophyController::get_carrier_speed_multiplier);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "trophy_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_trophy_scene", "get_trophy_scene");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "base_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_base_scene", "get_base_scene");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "carrier_speed_multiplier", PROPERTY_HINT_RANGE, "0.1,1,0.05"), "set_carrier_speed_multiplier", "get_carrier_speed_multiplier");

	ADD_SIGNAL(MethodInfo("trophy_taken", PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_NODE_TYPE, "Participant")));
	ADD_SIGNAL(MethodInfo("trophy_dropped", PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_NODE_TYPE, "Participant"), PropertyInfo(Variant::VECTOR2, "world_position")));
	ADD_GROUP("Robo", "steal_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "steal_enabled"), "set_steal_enabled", "is_steal_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "steal_immunity", PROPERTY_HINT_RANGE, "0,10,0.1,suffix:s"), "set_steal_immunity", "get_steal_immunity");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "steal_margin", PROPERTY_HINT_RANGE, "0,32,0.5,suffix:px"), "set_steal_margin", "get_steal_margin");
	ADD_SIGNAL(MethodInfo("trophy_stolen", PropertyInfo(Variant::OBJECT, "thief", PROPERTY_HINT_NODE_TYPE, "Participant"), PropertyInfo(Variant::OBJECT, "victim", PROPERTY_HINT_NODE_TYPE, "Participant")));
	ADD_SIGNAL(MethodInfo("trophy_delivered", PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_NODE_TYPE, "Participant")));
}

void TrophyController::start(Maze *p_maze, const TypedArray<Participant> &p_participants) {
	clear_views();
	for (Participant *p : participants_) {
		if (p != nullptr && p->get_stats() != nullptr) {
			p->get_stats()->remove_modifiers_from(CARRIER_SOURCE);
		}
	}
	participants_.clear();
	if (maze_ != nullptr && maze_->is_connected("map_changed", callable_mp(this, &TrophyController::on_map_changed))) {
		maze_->disconnect("map_changed", callable_mp(this, &TrophyController::on_map_changed));
	}
	maze_ = p_maze;
	if (maze_ == nullptr) {
		UtilityFunctions::push_error("TrophyController.start: falta el laberinto.");
		return;
	}
	for (int i = 0; i < p_participants.size(); i++) {
		participants_.push_back(Object::cast_to<Participant>(p_participants[i]));
	}

	// Zonas de resguardo: el cuadro de pasillo completo de cada base (se ajusta al ancho de pasillo).
	std::vector<lv::core::TileRect> zones;
	const TypedArray<Rect2i> base_zones = maze_->get_base_zones(static_cast<int>(participants_.size()));
	for (int i = 0; i < base_zones.size(); i++) {
		const Rect2i z = base_zones[i];
		zones.push_back({ z.position.x, z.position.y, z.size.x, z.size.y });
	}
	maze_->connect("map_changed", callable_mp(this, &TrophyController::on_map_changed));
	const Vector2i center = maze_->get_center_cell();
	tracker_.setup({ center.x, center.y }, zones, 1);

	// Vistas: una base por participante (con su color, del tamaño exacto de su zona) y el trofeo.
	for (size_t i = 0; i < zones.size(); i++) {
		if (base_scene_.is_null()) {
			break;
		}
		Node2D *view = Object::cast_to<Node2D>(base_scene_->instantiate());
		if (view == nullptr) {
			continue;
		}
		const Rect2 world = zone_world_rect(zones[i]);
		view->set_position(to_local(world.get_center()));
		view->set("size_px", world.size.x);
		if (participants_[i] != nullptr) {
			view->set("color", participants_[i]->get_color());
		}
		add_child(view);
		base_views_.push_back(view);
	}
	if (trophy_scene_.is_valid()) {
		trophy_view_ = Object::cast_to<Node2D>(trophy_scene_->instantiate());
		if (trophy_view_ != nullptr) {
			add_child(trophy_view_);
		}
	}
	update_trophy_view();
}

void TrophyController::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint() || maze_ == nullptr || is_finished()) {
		return;
	}
	lv::core::PerfScope perf("trophy.update");
	tracker_.tick(static_cast<float>(p_delta));
	for (size_t i = 0; i < participants_.size(); i++) {
		Participant *p = participants_[i];
		if (p == nullptr) {
			continue;
		}
		const Vector2i cell = maze_->world_to_cell(p->get_global_position());
		handle(tracker_.update(static_cast<int>(i), { cell.x, cell.y }));
		if (is_finished()) {
			break;
		}
	}
	if (!is_finished()) {
		check_steal();
	}
	update_trophy_view();
}

// Robo por contacto. Solo hay un portador, así que basta con comparar contra los demás (máx. 7).
void TrophyController::check_steal() {
	Participant *carrier = get_carrier();
	if (!steal_enabled_ || carrier == nullptr || tracker_.immunity_left() > 0.0f) {
		return;
	}
	const Vector2 at = carrier->get_global_position();
	for (size_t i = 0; i < participants_.size(); i++) {
		Participant *p = participants_[i];
		if (p == nullptr || p == carrier || !tracker_.can_steal(static_cast<int>(i))) {
			continue;
		}
		const float reach = carrier->get_body_radius() + p->get_body_radius() + steal_margin_;
		if (p->get_global_position().distance_squared_to(at) <= reach * reach) {
			const Vector2i cell = maze_->world_to_cell(at);
			handle(tracker_.steal(static_cast<int>(i), { cell.x, cell.y }));
			return;
		}
	}
}

void TrophyController::drop_trophy() {
	Participant *carrier = get_carrier();
	if (carrier == nullptr || maze_ == nullptr) {
		return;
	}
	const Vector2i cell = maze_->world_to_cell(carrier->get_global_position());
	handle(tracker_.drop({ cell.x, cell.y }));
	update_trophy_view();
}

void TrophyController::handle(const std::vector<TrophyTracker::Event> &p_events) {
	for (const TrophyTracker::Event &event : p_events) {
		Participant *p = participant(event.participant);
		StatsComponent *stats = p != nullptr ? p->get_stats() : nullptr;
		switch (event.type) {
			case TrophyTracker::Event::Type::Taken:
				if (stats != nullptr) {
					stats->add_modifier(StatsComponent::STAT_SPEED, StatsComponent::OP_MULTIPLY, carrier_speed_multiplier_, -1.0, CARRIER_SOURCE);
				}
				emit_signal("trophy_taken", p);
				break;
			case TrophyTracker::Event::Type::Dropped:
				if (stats != nullptr) {
					stats->remove_modifiers_from(CARRIER_SOURCE);
				}
				emit_signal("trophy_dropped", p, cell_center(event.cell));
				break;
			case TrophyTracker::Event::Type::Stolen: {
				// La lentitud del portador pasa de la víctima al ladrón.
				Participant *victim = participant(event.victim);
				if (victim != nullptr && victim->get_stats() != nullptr) {
					victim->get_stats()->remove_modifiers_from(CARRIER_SOURCE);
				}
				if (stats != nullptr) {
					stats->add_modifier(StatsComponent::STAT_SPEED, StatsComponent::OP_MULTIPLY, carrier_speed_multiplier_, -1.0, CARRIER_SOURCE);
				}
				emit_signal("trophy_stolen", p, victim);
				break;
			}
			case TrophyTracker::Event::Type::Delivered:
				if (stats != nullptr) {
					stats->remove_modifiers_from(CARRIER_SOURCE);
				}
				emit_signal("trophy_delivered", p);
				break;
		}
	}
}

void TrophyController::update_trophy_view() {
	if (trophy_view_ == nullptr) {
		return;
	}
	trophy_view_->set_global_position(get_trophy_position());
	trophy_view_->set("carried", tracker_.state() == TrophyTracker::State::Carried);
	trophy_view_->set_visible(tracker_.state() != TrophyTracker::State::Delivered);
}

Vector2 TrophyController::get_trophy_position() const {
	if (Participant *carrier = get_carrier()) {
		return carrier->get_global_position();
	}
	if (tracker_.state() == TrophyTracker::State::Delivered) {
		if (Participant *winner = get_winner()) {
			return winner->get_global_position();
		}
	}
	return maze_ != nullptr ? cell_center(tracker_.trophy_cell()) : Vector2();
}

Vector2 TrophyController::get_base_position(Participant *p_participant) const {
	for (size_t i = 0; i < participants_.size(); i++) {
		if (participants_[i] == p_participant && maze_ != nullptr) {
			return zone_world_rect(tracker_.base_of(static_cast<int>(i))).get_center();
		}
	}
	return Vector2();
}

Participant *TrophyController::get_carrier() const {
	return participant(tracker_.carrier());
}

Participant *TrophyController::get_winner() const {
	return participant(tracker_.winner());
}

Participant *TrophyController::participant(int p_index) const {
	return (p_index >= 0 && p_index < static_cast<int>(participants_.size())) ? participants_[p_index] : nullptr;
}

Vector2 TrophyController::cell_center(const lv::core::Cell &p_cell) const {
	return maze_->cell_to_world(Vector2i(p_cell.x, p_cell.y));
}

Rect2 TrophyController::zone_world_rect(const lv::core::TileRect &p_zone) const {
	return maze_->tile_rect_to_world(Rect2i(p_zone.x, p_zone.y, p_zone.w, p_zone.h));
}

void TrophyController::clear_views() {
	for (Node2D *view : base_views_) {
		view->queue_free();
	}
	base_views_.clear();
	if (trophy_view_ != nullptr) {
		trophy_view_->queue_free();
		trophy_view_ = nullptr;
	}
}

// Si una pared se cierra donde está el trofeo (en el piso), se mueve al suelo libre más cercano.
void TrophyController::on_map_changed(const TypedArray<Vector2i> &p_tiles, bool p_now_wall) {
	const TrophyTracker::State state = tracker_.state();
	if (!p_now_wall || (state != TrophyTracker::State::InVault && state != TrophyTracker::State::Dropped)) {
		return;
	}
	const lv::core::Cell current = tracker_.trophy_cell();
	if (!p_tiles.has(Vector2i(current.x, current.y))) {
		return;
	}
	const lv::core::Cell free = maze_->get_grid().nearest_floor(current);
	tracker_.relocate(free);
	update_trophy_view();
}