#include "game/stats_component.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace {
// Límites de seguridad para que ningún efecto deje al participante inmóvil o ciego del todo.
constexpr float SPEED_MIN = 20.0f;
constexpr float SPEED_MAX = 600.0f;
constexpr float VISION_MIN = 1.0f;
constexpr float VISION_MAX = 20.0f;

lv::core::StatType to_core(StatsComponent::Stat p_stat) {
	return static_cast<lv::core::StatType>(p_stat);
}
} // namespace

void StatsComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_stat", "stat"), &StatsComponent::get_stat);
	ClassDB::bind_method(D_METHOD("get_speed"), &StatsComponent::get_speed);
	ClassDB::bind_method(D_METHOD("get_vision"), &StatsComponent::get_vision);
	ClassDB::bind_method(D_METHOD("is_bonus_maxed", "stat"), &StatsComponent::is_bonus_maxed);
	ClassDB::bind_method(D_METHOD("add_modifier", "stat", "op", "value", "duration", "source"), &StatsComponent::add_modifier);
	ClassDB::bind_method(D_METHOD("remove_modifier", "id"), &StatsComponent::remove_modifier);
	ClassDB::bind_method(D_METHOD("remove_modifiers_from", "source"), &StatsComponent::remove_modifiers_from);
	ClassDB::bind_method(D_METHOD("get_modifiers"), &StatsComponent::get_modifiers);
	ClassDB::bind_method(D_METHOD("set_profile", "profile"), &StatsComponent::set_profile);
	ClassDB::bind_method(D_METHOD("get_profile"), &StatsComponent::get_profile);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "profile", PROPERTY_HINT_RESOURCE_TYPE, "StatsProfile"), "set_profile", "get_profile");

	BIND_ENUM_CONSTANT(STAT_SPEED);
	BIND_ENUM_CONSTANT(STAT_VISION);
	BIND_ENUM_CONSTANT(OP_ADD);
	BIND_ENUM_CONSTANT(OP_MULTIPLY);

	ADD_SIGNAL(MethodInfo("stat_changed", PropertyInfo(Variant::INT, "stat"), PropertyInfo(Variant::FLOAT, "value")));
	ADD_SIGNAL(MethodInfo("modifiers_changed"));
}

void StatsComponent::_ready() {
	stats_.set_limits(lv::core::StatType::Speed, SPEED_MIN, SPEED_MAX);
	stats_.set_limits(lv::core::StatType::Vision, VISION_MIN, VISION_MAX);
	apply_profile();
}

void StatsComponent::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint() || stats_.modifiers().empty()) {
		return;
	}
	const std::vector<lv::core::StatType> changed = stats_.tick(p_delta);
	for (const lv::core::StatType stat : changed) {
		notify(stat);
	}
	if (!changed.empty()) {
		emit_signal("modifiers_changed");
	}
}

float StatsComponent::get_stat(Stat p_stat) const {
	return stats_.get(to_core(p_stat));
}

int StatsComponent::add_modifier(Stat p_stat, Op p_op, float p_value, double p_duration, const String &p_source) {
	lv::core::StatModifier modifier;
	modifier.stat = to_core(p_stat);
	modifier.op = static_cast<lv::core::ModifierOp>(p_op);
	modifier.value = p_value;
	modifier.duration = p_duration < 0.0 ? lv::core::StatModifier::PERMANENT : p_duration;
	modifier.source = p_source.utf8().get_data();
	const int id = stats_.add_modifier(modifier);
	if (id == 0) {
		return 0; // tope de mejora alcanzado: no cambió nada
	}
	notify(modifier.stat);
	emit_signal("modifiers_changed");
	return id;
}

bool StatsComponent::remove_modifier(int p_id) {
	for (const auto &active : stats_.modifiers()) {
		if (active.id == p_id) {
			const lv::core::StatType stat = active.modifier.stat;
			stats_.remove_modifier(p_id);
			notify(stat);
			emit_signal("modifiers_changed");
			return true;
		}
	}
	return false;
}

int StatsComponent::remove_modifiers_from(const String &p_source) {
	const int removed = stats_.remove_modifiers_from(p_source.utf8().get_data());
	if (removed > 0) {
		notify(lv::core::StatType::Speed);
		notify(lv::core::StatType::Vision);
		emit_signal("modifiers_changed");
	}
	return removed;
}

Array StatsComponent::get_modifiers() const {
	Array result;
	for (const auto &active : stats_.modifiers()) {
		Dictionary d;
		d["id"] = active.id;
		d["stat"] = static_cast<int>(active.modifier.stat);
		d["op"] = static_cast<int>(active.modifier.op);
		d["value"] = active.modifier.value;
		d["remaining"] = active.remaining;
		d["duration"] = active.modifier.duration;
		d["source"] = String::utf8(active.modifier.source.c_str());
		result.push_back(d);
	}
	return result;
}

void StatsComponent::set_profile(const Ref<StatsProfile> &p_profile) {
	profile_ = p_profile;
	if (is_inside_tree()) {
		apply_profile();
	}
}

void StatsComponent::apply_profile() {
	// Sin perfil se usan los valores por defecto de StatsProfile.
	Ref<StatsProfile> profile = profile_;
	if (profile.is_null()) {
		profile.instantiate();
	}
	stats_.set_base(lv::core::StatType::Speed, profile->get_speed());
	stats_.set_base(lv::core::StatType::Vision, profile->get_vision());
	stats_.set_bonus_cap(lv::core::StatType::Speed, profile->get_speed_bonus_max());
	stats_.set_bonus_cap(lv::core::StatType::Vision, profile->get_vision_bonus_max());
	notify(lv::core::StatType::Speed);
	notify(lv::core::StatType::Vision);
}

void StatsComponent::notify(lv::core::StatType p_stat) {
	emit_signal("stat_changed", static_cast<int>(p_stat), stats_.get(p_stat));
}
