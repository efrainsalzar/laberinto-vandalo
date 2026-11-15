#include "game/trophy_controller.h"

#include "core/perf.h"

#include "game/life_component.h"
#include "game/maze.h"
#include "game/participant.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using lv::core::TrophyTracker;

void TrophyRules::_bind_methods() {
#define LV_BIND(name, variant, hint)                                                 \
	ClassDB::bind_method(D_METHOD("set_" #name, "value"), &TrophyRules::set_##name); \
	ClassDB::bind_method(D_METHOD("get_" #name), &TrophyRules::get_##name);          \
	ADD_PROPERTY(PropertyInfo(Variant::variant, #name, PROPERTY_HINT_RANGE, hint), "set_" #name, "get_" #name);
	ADD_GROUP("Portador", "");
	LV_BIND(carrier_speed_multiplier, FLOAT, "0.1,1,0.05")
	ADD_GROUP("Robo", "");
	ClassDB::bind_method(D_METHOD("set_steal_enabled", "value"), &TrophyRules::set_steal_enabled);
	ClassDB::bind_method(D_METHOD("get_steal_enabled"), &TrophyRules::get_steal_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "steal_enabled"), "set_steal_enabled", "get_steal_enabled");
	ClassDB::bind_method(D_METHOD("set_steal_mode", "value"), &TrophyRules::set_steal_mode);
	ClassDB::bind_method(D_METHOD("get_steal_mode"), &TrophyRules::get_steal_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "steal_mode", PROPERTY_HINT_ENUM, "Transferir,Eliminar al portador"), "set_steal_mode", "get_steal_mode");
	LV_BIND(steal_immunity, FLOAT, "0,10,0.1,suffix:s")
	LV_BIND(drop_lock, FLOAT, "0,5,0.05,suffix:s")
	LV_BIND(steal_margin, FLOAT, "0,32,0.5,suffix:px")
	ADD_GROUP("Modo Transferir: anti-bucle", "");
	LV_BIND(thief_boost_multiplier, FLOAT, "1,3,0.05")
	LV_BIND(thief_boost_seconds, FLOAT, "0,10,0.1,suffix:s")
	LV_BIND(victim_stun_multiplier, FLOAT, "0,1,0.05")
	LV_BIND(victim_stun_seconds, FLOAT, "0,10,0.1,suffix:s")
	ADD_GROUP("IA", "");
	LV_BIND(max_chasers, INT, "0,7,1")
#undef LV_BIND
	BIND_ENUM_CONSTANT(STEAL_TRANSFER);
	BIND_ENUM_CONSTANT(STEAL_ELIMINATE);
}

void TrophyController::set_rules(const Ref<TrophyRules> &p_value) {
	rules_ = p_value;
	tracker_.set_steal_immunity(rules().steal_immunity);
	tracker_.set_drop_lock(rules().drop_lock);
}

const TrophyRules &TrophyController::rules() const {
	if (rules_.is_valid()) {
		return **rules_;
	}
	if (default_rules_.is_null()) {
		const_cast<TrophyController *>(this)->default_rules_.instantiate();
	}
	return **default_rules_;
}

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
	ClassDB::bind_method(D_METHOD("get_immunity_left"), &TrophyController::get_immunity_left);
	ClassDB::bind_method(D_METHOD("set_base_scene", "value"), &TrophyController::set_base_scene);
	ClassDB::bind_method(D_METHOD("get_base_scene"), &TrophyController::get_base_scene);
	ClassDB::bind_method(D_METHOD("set_rules", "value"), &TrophyController::set_rules);
	ClassDB::bind_method(D_METHOD("get_rules"), &TrophyController::get_rules);
	ClassDB::bind_method(D_METHOD("get_steal_immunity"), &TrophyController::get_steal_immunity);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "trophy_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_trophy_scene", "get_trophy_scene");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "base_scene", PROPERTY_HINT_RESOURCE_TYPE, "PackedScene"), "set_base_scene", "get_base_scene");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "rules", PROPERTY_HINT_RESOURCE_TYPE, "TrophyRules"), "set_rules", "get_rules");

	ADD_SIGNAL(MethodInfo("trophy_taken", PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_NODE_TYPE, "Participant")));
	ADD_SIGNAL(MethodInfo("trophy_dropped", PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_NODE_TYPE, "Participant"), PropertyInfo(Variant::VECTOR2, "world_position")));
	ADD_SIGNAL(MethodInfo("trophy_stolen", PropertyInfo(Variant::OBJECT, "thief", PROPERTY_HINT_NODE_TYPE, "Participant"), PropertyInfo(Variant::OBJECT, "victim", PROPERTY_HINT_NODE_TYPE, "Participant")));
	ADD_SIGNAL(MethodInfo("trophy_delivered", PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_NODE_TYPE, "Participant")));
}

void TrophyController::start(Maze *p_maze, const TypedArray<Participant> &p_participants) {
	clear_views();
	for (Participant *p : participants_) {
		if (p != nullptr && p->get_stats() != nullptr) {
			p->get_stats()->remove_modifiers_from(CARRIER_SOURCE);
			p->get_stats()->remove_modifiers_from(BOOST_SOURCE);
			p->get_stats()->remove_modifiers_from(STUN_SOURCE);
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
	tracker_.set_steal_immunity(rules().steal_immunity);
	tracker_.set_drop_lock(rules().drop_lock);

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
		if (p == nullptr || !p->is_active()) {
			continue; // eliminado: no toma ni entrega nada hasta reaparecer
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
	if (!rules().steal_enabled || carrier == nullptr || tracker_.immunity_left() > 0.0f) {
		return;
	}
	const bool eliminate = rules().steal_mode == TrophyRules::STEAL_ELIMINATE;
	if (eliminate && !carrier->is_vulnerable()) {
		return; // recién reaparecido: protegido
	}
	const Vector2 at = carrier->get_global_position();
	for (size_t i = 0; i < participants_.size(); i++) {
		Participant *p = participants_[i];
		if (p == nullptr || p == carrier || !p->is_active() || !tracker_.can_steal(static_cast<int>(i))) {
			continue;
		}
		const float reach = carrier->get_body_radius() + p->get_body_radius() + rules().steal_margin;
		if (p->get_global_position().distance_squared_to(at) <= reach * reach) {
			if (eliminate) {
				eliminate_carrier(carrier, p);
				return;
			}
			const Vector2i cell = maze_->world_to_cell(at);
			handle(tracker_.steal(static_cast<int>(i), { cell.x, cell.y }));
			return;
		}
	}
}

// Modo Eliminar: el trofeo cae donde estaba el portador y el portador sale del juego hasta reaparecer en su base.
void TrophyController::eliminate_carrier(Participant *p_carrier, Participant *p_by) {
	const Vector2i cell = maze_->world_to_cell(p_carrier->get_global_position());
	const lv::core::Cell floor = maze_->get_grid().nearest_floor({ cell.x, cell.y });
	handle(tracker_.drop(floor));
	if (LifeComponent *life = p_carrier->get_life()) {
		life->eliminate(p_by);
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
					stats->add_modifier(StatsComponent::STAT_SPEED, StatsComponent::OP_MULTIPLY, rules().carrier_speed_multiplier, -1.0, CARRIER_SOURCE);
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
					stats->add_modifier(StatsComponent::STAT_SPEED, StatsComponent::OP_MULTIPLY, rules().carrier_speed_multiplier, -1.0, CARRIER_SOURCE);
				}
				// Anti-bucle: el ladrón escapa (impulso) y la víctima queda aturdida un momento.
				const TrophyRules &r = rules();
				if (stats != nullptr && r.thief_boost_seconds > 0.0f) {
					stats->remove_modifiers_from(STUN_SOURCE); // si lo habían aturdido antes, ya no
					stats->add_modifier(StatsComponent::STAT_SPEED, StatsComponent::OP_MULTIPLY, r.thief_boost_multiplier, r.thief_boost_seconds, BOOST_SOURCE);
				}
				if (victim != nullptr && victim->get_stats() != nullptr && r.victim_stun_seconds > 0.0f) {
					victim->get_stats()->remove_modifiers_from(BOOST_SOURCE);
					victim->get_stats()->add_modifier(StatsComponent::STAT_SPEED, StatsComponent::OP_MULTIPLY, r.victim_stun_multiplier, r.victim_stun_seconds, STUN_SOURCE);
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