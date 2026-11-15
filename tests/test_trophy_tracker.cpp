#include <doctest/doctest.h>

#include "core/trophy_tracker.h"

using lv::core::Cell;
using lv::core::TrophyTracker;
using State = TrophyTracker::State;
using Type = TrophyTracker::Event::Type;

namespace {
// Trofeo en (20,20); base del participante 0 en (1,1) y del 1 en (40,1).
TrophyTracker make() {
	TrophyTracker t;
	t.setup({ 20, 20 }, { { 1, 1 }, { 40, 1 } }, 1, 1);
	return t;
}
} // namespace

TEST_SUITE("TrophyTracker") {
	TEST_CASE("Empieza en la bóveda, sin portador ni ganador") {
		const TrophyTracker t = make();
		CHECK(t.state() == State::InVault);
		CHECK(t.carrier() == TrophyTracker::NO_ONE);
		CHECK(t.winner() == TrophyTracker::NO_ONE);
	}

	TEST_CASE("Se toma al llegar a su lado, no desde lejos") {
		TrophyTracker t = make();
		CHECK(t.update(0, { 17, 20 }).empty());
		const auto events = t.update(0, { 21, 21 });
		REQUIRE(events.size() == 1);
		CHECK(events[0].type == Type::Taken);
		CHECK(events[0].participant == 0);
		CHECK(t.state() == State::Carried);
		CHECK(t.carrier() == 0);
	}

	TEST_CASE("Mientras alguien lo lleva, nadie más lo toma") {
		TrophyTracker t = make();
		t.update(0, { 20, 20 });
		CHECK(t.update(1, { 20, 20 }).empty());
		CHECK(t.carrier() == 0);
	}

	TEST_CASE("Solo se entrega en la base propia") {
		TrophyTracker t = make();
		t.update(0, { 20, 20 });
		CHECK(t.update(0, { 40, 1 }).empty()); // base del rival: nada
		CHECK(t.state() == State::Carried);
		const auto events = t.update(0, { 2, 2 }); // su base (radio 1)
		REQUIRE(events.size() == 1);
		CHECK(events[0].type == Type::Delivered);
		CHECK(t.state() == State::Delivered);
		CHECK(t.winner() == 0);
	}

	TEST_CASE("Llegar a la base sin el trofeo no hace nada") {
		TrophyTracker t = make();
		CHECK(t.update(0, { 1, 1 }).empty());
		t.update(1, { 20, 20 }); // lo toma el otro
		CHECK(t.update(0, { 1, 1 }).empty());
		CHECK(t.winner() == TrophyTracker::NO_ONE);
	}

	TEST_CASE("Si se suelta, cae al piso y cualquiera puede tomarlo ahí") {
		TrophyTracker t = make();
		t.update(0, { 20, 20 });
		const auto dropped = t.drop({ 10, 12 });
		REQUIRE(dropped.size() == 1);
		CHECK(dropped[0].type == Type::Dropped);
		CHECK(dropped[0].participant == 0);
		CHECK(t.state() == State::Dropped);
		CHECK(t.trophy_cell() == Cell{ 10, 12 });
		CHECK(t.update(1, { 20, 20 }).empty()); // ya no está en la bóveda
		const auto taken = t.update(1, { 10, 13 });
		REQUIRE(taken.size() == 1);
		CHECK(t.carrier() == 1);
	}

	TEST_CASE("Soltar sin portador no hace nada") {
		TrophyTracker t = make();
		CHECK(t.drop({ 5, 5 }).empty());
		CHECK(t.state() == State::InVault);
	}

	TEST_CASE("Tras la entrega, la partida queda cerrada") {
		TrophyTracker t = make();
		t.update(0, { 20, 20 });
		t.update(0, { 1, 1 });
		CHECK(t.update(1, { 20, 20 }).empty());
		CHECK(t.drop({ 3, 3 }).empty());
		CHECK(t.winner() == 0);
	}

	TEST_CASE("Participantes inválidos se ignoran") {
		TrophyTracker t = make();
		CHECK(t.update(-1, { 20, 20 }).empty());
		CHECK(t.update(5, { 20, 20 }).empty());
		CHECK_FALSE(t.is_in_own_base(7, { 1, 1 }));
	}
}
