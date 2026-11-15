#include <doctest/doctest.h>

#include "core/stats.h"

using lv::core::ModifierOp;
using lv::core::StatModifier;
using lv::core::Stats;
using lv::core::StatType;

namespace {
StatModifier mod(StatType p_stat, ModifierOp p_op, float p_value, double p_duration = StatModifier::PERMANENT, const std::string &p_source = "") {
	StatModifier m;
	m.stat = p_stat;
	m.op = p_op;
	m.value = p_value;
	m.duration = p_duration;
	m.source = p_source;
	return m;
}
} // namespace

TEST_SUITE("Stats") {
	TEST_CASE("Sin modificadores, el valor es el base") {
		Stats s;
		s.set_base(StatType::Speed, 160.0f);
		CHECK(s.get(StatType::Speed) == doctest::Approx(160.0f));
		CHECK(s.get(StatType::Vision) == doctest::Approx(0.0f));
	}

	TEST_CASE("Primero se suman, después se multiplica") {
		Stats s;
		s.set_base(StatType::Speed, 160.0f);
		s.add_modifier(mod(StatType::Speed, ModifierOp::Multiply, 1.5f));
		s.add_modifier(mod(StatType::Speed, ModifierOp::Add, 10.0f));
		s.add_modifier(mod(StatType::Speed, ModifierOp::Multiply, 0.6f));
		// (160 + 10) × 1.5 × 0.6 = 153
		CHECK(s.get(StatType::Speed) == doctest::Approx(153.0f));
	}

	TEST_CASE("Un modificador solo afecta a su stat") {
		Stats s;
		s.set_base(StatType::Speed, 100.0f);
		s.set_base(StatType::Vision, 5.0f);
		s.add_modifier(mod(StatType::Vision, ModifierOp::Add, 2.0f));
		CHECK(s.get(StatType::Speed) == doctest::Approx(100.0f));
		CHECK(s.get(StatType::Vision) == doctest::Approx(7.0f));
	}

	TEST_CASE("Los temporales vencen y tick avisa qué stat cambió") {
		Stats s;
		s.set_base(StatType::Speed, 100.0f);
		s.add_modifier(mod(StatType::Speed, ModifierOp::Multiply, 2.0f, 5.0));
		s.add_modifier(mod(StatType::Speed, ModifierOp::Add, 10.0f)); // permanente
		CHECK(s.get(StatType::Speed) == doctest::Approx(220.0f));

		CHECK(s.tick(4.0).empty());
		CHECK(s.get(StatType::Speed) == doctest::Approx(220.0f));

		const auto changed = s.tick(1.5);
		REQUIRE(changed.size() == 1);
		CHECK(changed[0] == StatType::Speed);
		CHECK(s.get(StatType::Speed) == doctest::Approx(110.0f));
		CHECK(s.modifiers().size() == 1); // el permanente sigue
	}

	TEST_CASE("Se puede quitar por id o por origen") {
		Stats s;
		s.set_base(StatType::Speed, 100.0f);
		const int id = s.add_modifier(mod(StatType::Speed, ModifierOp::Add, 50.0f));
		s.add_modifier(mod(StatType::Speed, ModifierOp::Multiply, 0.6f, StatModifier::PERMANENT, "trophy"));
		s.add_modifier(mod(StatType::Vision, ModifierOp::Add, 1.0f, StatModifier::PERMANENT, "trophy"));

		CHECK(s.remove_modifier(id));
		CHECK_FALSE(s.remove_modifier(id));
		CHECK(s.get(StatType::Speed) == doctest::Approx(60.0f));

		CHECK(s.remove_modifiers_from("trophy") == 2);
		CHECK(s.get(StatType::Speed) == doctest::Approx(100.0f));
		CHECK(s.modifiers().empty());
	}

	TEST_CASE("Los límites recortan el valor final") {
		Stats s;
		s.set_base(StatType::Speed, 100.0f);
		s.set_limits(StatType::Speed, 20.0f, 300.0f);
		s.add_modifier(mod(StatType::Speed, ModifierOp::Multiply, 10.0f));
		CHECK(s.get(StatType::Speed) == doctest::Approx(300.0f));
		s.add_modifier(mod(StatType::Speed, ModifierOp::Multiply, 0.0f));
		CHECK(s.get(StatType::Speed) == doctest::Approx(20.0f));
	}
}
