#include <doctest/doctest.h>

#include "core/explosives.h"

using namespace lv::core;

namespace {
ExplosiveSpec bomb() {
	ExplosiveSpec s;
	s.kind = ExplosiveKind::Bomb;
	s.fuse = 2.0f;
	return s;
}
ExplosiveSpec mine() {
	ExplosiveSpec s;
	s.kind = ExplosiveKind::Mine;
	s.arm_time = 1.0f;
	s.trigger_radius = 0.8f;
	return s;
}
const std::vector<ProjectileTarget> NOBODY;
} // namespace

TEST_SUITE("Bombas y minas (ExplosiveWorld)") {
	TEST_CASE("La bomba explota a los 2 s, donde la dejaron") {
		ExplosiveWorld world;
		world.place(5.5f, 3.5f, bomb(), 0, 7);
		std::vector<ExplosiveEvent> events;
		world.step(1.9f, NOBODY, events);
		CHECK(events.empty());
		CHECK(world.size() == 1);
		world.step(0.2f, NOBODY, events);
		REQUIRE(events.size() == 1);
		CHECK(events[0].x == doctest::Approx(5.5f));
		CHECK(events[0].owner == 0);
		CHECK(events[0].tag == 7);
		CHECK(events[0].explosion_radius == doctest::Approx(2.0f));
		CHECK(world.size() == 0);
	}

	TEST_CASE("La mina no explota antes de armarse") {
		ExplosiveWorld world;
		world.place(0.0f, 0.0f, mine(), 0);
		const std::vector<ProjectileTarget> rival = { { 1, 0.2f, 0.0f, 0.4f } };
		std::vector<ExplosiveEvent> events;
		world.step(0.5f, rival, events);
		CHECK(events.empty());
		world.step(0.6f, rival, events); // se arma en este paso: todavía no explota
		CHECK(events.empty());
		world.step(0.1f, rival, events);
		REQUIRE(events.size() == 1);
		CHECK(events[0].trigger == 1);
		CHECK(events[0].kind == ExplosiveKind::Mine);
	}

	TEST_CASE("La mina armada no explota por su dueño ni por alguien lejos") {
		ExplosiveWorld world;
		world.place(0.0f, 0.0f, mine(), 0);
		std::vector<ExplosiveEvent> events;
		world.step(2.0f, NOBODY, events);
		const std::vector<ProjectileTarget> around = {
			{ 0, 0.0f, 0.0f, 0.4f }, // el dueño, encima
			{ 1, 1.3f, 0.0f, 0.4f }, // fuera de 0,8 + 0,4
		};
		world.step(0.1f, around, events);
		CHECK(events.empty());
		CHECK(world.size() == 1);
		const std::vector<ProjectileTarget> close = { { 1, 1.1f, 0.0f, 0.4f } };
		world.step(0.1f, close, events);
		CHECK(events.size() == 1);
	}

	TEST_CASE("Tope: con el arreglo lleno desaparece el más viejo") {
		ExplosiveWorld world(3);
		for (int i = 0; i < 5; i++) {
			world.place(static_cast<float>(i), 0.0f, mine(), i, i);
		}
		REQUIRE(world.size() == 3);
		CHECK(world.items()[0].tag == 2);
		CHECK(world.items()[2].tag == 4);
	}

	TEST_CASE("Al explotar uno, los demás conservan su orden") {
		ExplosiveWorld world;
		ExplosiveSpec quick = bomb();
		quick.fuse = 0.5f;
		world.place(0.0f, 0.0f, bomb(), 0, 1);
		world.place(1.0f, 0.0f, quick, 0, 2);
		world.place(2.0f, 0.0f, bomb(), 0, 3);
		std::vector<ExplosiveEvent> events;
		world.step(0.6f, NOBODY, events);
		REQUIRE(events.size() == 1);
		CHECK(events[0].tag == 2);
		REQUIRE(world.size() == 2);
		CHECK(world.items()[0].tag == 1);
		CHECK(world.items()[1].tag == 3);
	}
}
