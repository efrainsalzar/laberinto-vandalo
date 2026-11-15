#include <doctest/doctest.h>

#include "core/life_cycle.h"

using lv::core::LifeCycle;
using State = LifeCycle::State;
using Event = LifeCycle::Event;

TEST_SUITE("LifeCycle (eliminar y reaparecer)") {
	TEST_CASE("Vivo → eliminado → protegido → vivo, con sus tiempos") {
		LifeCycle life;
		life.setup(3.0f, 2.0f);
		CHECK(life.is_vulnerable());
		CHECK(life.eliminate());
		CHECK(life.state() == State::Eliminated);
		CHECK_FALSE(life.is_active());
		CHECK(life.tick(2.9f).empty());
		auto events = life.tick(0.2f);
		REQUIRE(events.size() == 1);
		CHECK(events[0] == Event::Respawned);
		CHECK(life.state() == State::Protected);
		CHECK(life.is_active());
		CHECK_FALSE(life.is_vulnerable());
		CHECK(life.time_left() == doctest::Approx(1.9f)); // 0,1 s del tramo ya cuenta como protección
		events = life.tick(2.0f);
		REQUIRE(events.size() == 1);
		CHECK(events[0] == Event::ProtectionEnded);
		CHECK(life.is_vulnerable());
	}

	TEST_CASE("No se puede eliminar a quien ya está eliminado o protegido") {
		LifeCycle life;
		life.setup(1.0f, 1.0f);
		CHECK(life.eliminate());
		CHECK_FALSE(life.eliminate());
		life.tick(1.0f);
		CHECK(life.state() == State::Protected);
		CHECK_FALSE(life.eliminate());
	}

	TEST_CASE("Un frame largo recorre los dos tramos y emite ambos eventos en orden") {
		LifeCycle life;
		life.setup(1.0f, 1.0f);
		life.eliminate();
		const auto events = life.tick(5.0f);
		REQUIRE(events.size() == 2);
		CHECK(events[0] == Event::Respawned);
		CHECK(events[1] == Event::ProtectionEnded);
		CHECK(life.state() == State::Alive);
	}

	TEST_CASE("Sin protección: vuelve directo a vivo") {
		LifeCycle life;
		life.setup(0.5f, 0.0f);
		life.eliminate();
		const auto events = life.tick(0.5f);
		REQUIRE(events.size() == 2);
		CHECK(life.is_vulnerable());
	}
}
