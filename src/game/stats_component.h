#pragma once

#include "core/stats.h"
#include "game/stats_profile.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace godot;

// Componente de stats de un participante (nodo hijo). Composición: el participante *tiene* stats.
//
// - Envuelve a lv::core::Stats (la lógica y los tests viven ahí).
// - Toma los valores base de un StatsProfile (datos).
// - Observer: emite `stat_changed` cuando un valor cambia, para que la interfaz,
//   la visión, etc. reaccionen sin que este componente los conozca.
// Ver docs/ARQUITECTURA.md §6.
class StatsComponent : public Node {
	GDCLASS(StatsComponent, Node)

public:
	enum Stat {
		STAT_SPEED = static_cast<int>(lv::core::StatType::Speed),
		STAT_VISION = static_cast<int>(lv::core::StatType::Vision),
	};
	enum Op {
		OP_ADD = static_cast<int>(lv::core::ModifierOp::Add),
		OP_MULTIPLY = static_cast<int>(lv::core::ModifierOp::Multiply),
	};

	void _ready() override;
	void _process(double p_delta) override;

	float get_stat(Stat p_stat) const;
	float get_speed() const { return get_stat(STAT_SPEED); }
	float get_vision() const { return get_stat(STAT_VISION); }
	// ¿Ya llegó al tope de mejoras permanentes (StatsProfile.*_bonus_max)?
	bool is_bonus_maxed(Stat p_stat) const { return stats_.is_bonus_maxed(static_cast<lv::core::StatType>(p_stat)); }

	// duration < 0 = permanente. Devuelve el id del modificador.
	int add_modifier(Stat p_stat, Op p_op, float p_value, double p_duration, const String &p_source);
	bool remove_modifier(int p_id);
	int remove_modifiers_from(const String &p_source);
	// Lista de diccionarios {id, stat, op, value, remaining, duration, source} (para la interfaz).
	Array get_modifiers() const;

	void set_profile(const Ref<StatsProfile> &p_profile);
	Ref<StatsProfile> get_profile() const { return profile_; }

protected:
	static void _bind_methods();

private:
	lv::core::Stats stats_;
	Ref<StatsProfile> profile_;

	void apply_profile();
	void notify(lv::core::StatType p_stat);
};

VARIANT_ENUM_CAST(StatsComponent::Stat);
VARIANT_ENUM_CAST(StatsComponent::Op);
