#include <doctest/doctest.h>

#include "core/match_flow.h"

using lv::core::MatchFlow;
using Phase = MatchFlow::Phase;
using Outcome = MatchFlow::Outcome;
using Type = MatchFlow::Event::Type;

namespace {
// 100 s: Preparación 0-40, Asalto 40-75, Golpe 75-100. Avisos a 60 y 10 s.
MatchFlow make(float p_duration = 100.0f) {
	MatchFlow flow;
	MatchFlow::Rules rules;
	rules.duration = p_duration;
	flow.start(rules);
	return flow;
}
} // namespace

TEST_SUITE("MatchFlow") {
	TEST_CASE("Empieza en Preparación con el reloj completo") {
		const MatchFlow flow = make();
		CHECK(flow.phase() == Phase::Preparation);
		CHECK(flow.is_running());
		CHECK(flow.time_left() == doctest::Approx(100.0f));
		CHECK(flow.phase_time_left() == doctest::Approx(40.0f));
		CHECK(flow.phase_start(Phase::Assault) == doctest::Approx(40.0f));
		CHECK(flow.phase_start(Phase::Heist) == doctest::Approx(75.0f));
	}

	TEST_CASE("Cambia de fase justo en el límite, no antes") {
		MatchFlow flow;
		MatchFlow::Rules rules;
		rules.duration = 100.0f;
		rules.warnings.clear();
		flow.start(rules);
		CHECK(flow.advance(39.9f).empty());
		const auto events = flow.advance(0.1f);
		REQUIRE(events.size() == 1);
		CHECK(events[0].type == Type::PhaseChanged);
		CHECK(events[0].phase == Phase::Assault);
		CHECK(flow.phase() == Phase::Assault);
	}

	TEST_CASE("Un frame largo que cruza dos fases emite ambos cambios en orden") {
		MatchFlow flow = make();
		const auto events = flow.advance(80.0f); // pasa por 40 y 75; aviso de 60 s cae en 40
		REQUIRE(events.size() == 3);
		CHECK(events[0].type == Type::TimeWarning); // a los 40 s quedan 60 (mismo instante, antes del cambio)
		CHECK(events[0].seconds_left == doctest::Approx(60.0f));
		CHECK(events[1].type == Type::PhaseChanged);
		CHECK(events[1].phase == Phase::Assault);
		CHECK(events[2].type == Type::PhaseChanged);
		CHECK(events[2].phase == Phase::Heist);
		CHECK(flow.elapsed() == doctest::Approx(80.0f));
	}

	TEST_CASE("Fin por tiempo: los avisos salen una sola vez y antes del fin") {
		MatchFlow flow = make();
		int warnings = 0;
		bool ended = false;
		for (int i = 0; i < 2000 && !ended; i++) {
			for (const auto &e : flow.advance(0.1f)) {
				if (e.type == Type::TimeWarning) {
					warnings++;
					CHECK_FALSE(ended);
				}
				if (e.type == Type::Ended) {
					ended = true;
					CHECK(e.outcome == Outcome::TimeUp);
				}
			}
		}
		CHECK(ended);
		CHECK(warnings == 2);
		CHECK(flow.outcome() == Outcome::TimeUp);
		CHECK(flow.winner() == -1);
		CHECK(flow.time_left() == doctest::Approx(0.0f));
	}

	TEST_CASE("Victoria en cualquier fase termina la partida") {
		for (float t : { 10.0f, 50.0f, 90.0f }) {
			CAPTURE(t);
			MatchFlow flow = make();
			flow.advance(t);
			const auto events = flow.declare_winner(2);
			REQUIRE(events.size() == 1);
			CHECK(events[0].type == Type::Ended);
			CHECK(events[0].outcome == Outcome::Victory);
			CHECK(events[0].winner == 2);
			CHECK(flow.phase() == Phase::Ended);
		}
	}

	TEST_CASE("Después del fin no pasa nada más") {
		MatchFlow flow = make();
		flow.declare_winner(0);
		CHECK(flow.advance(500.0f).empty());
		CHECK(flow.declare_winner(1).empty());
		CHECK(flow.skip_phase().empty());
		CHECK(flow.winner() == 0);
		CHECK(flow.outcome() == Outcome::Victory);
	}

	TEST_CASE("Empate: ganar en el último instante cuenta como victoria") {
		MatchFlow flow = make();
		flow.advance(99.95f);
		CHECK(flow.declare_winner(0).size() == 1);
		CHECK(flow.advance(0.1f).empty()); // el reloj ya no termina la partida
		CHECK(flow.outcome() == Outcome::Victory);
	}

	TEST_CASE("Sin límite: las fases avanzan, el Golpe no termina y no hay avisos") {
		MatchFlow flow;
		MatchFlow::Rules rules;
		rules.duration = 0.0f;
		rules.unlimited_reference = 100.0f;
		flow.start(rules);
		CHECK(flow.is_unlimited());
		CHECK(flow.time_left() < 0.0f);
		auto events = flow.advance(10000.0f);
		REQUIRE(events.size() == 2);
		CHECK(events[0].phase == Phase::Assault);
		CHECK(events[1].phase == Phase::Heist);
		CHECK(flow.is_running());
		CHECK(flow.phase_time_left() < 0.0f);
		CHECK(flow.skip_phase().empty());
	}

	TEST_CASE("Saltar fase: va al comienzo de la siguiente; en el Golpe termina por tiempo") {
		MatchFlow flow = make();
		auto events = flow.skip_phase(); // también sale el aviso de 60 s (cae justo a los 40)
		REQUIRE_FALSE(events.empty());
		CHECK(events.back().type == Type::PhaseChanged);
		CHECK(events.back().phase == Phase::Assault);
		CHECK(flow.elapsed() == doctest::Approx(40.0f));
		flow.skip_phase();
		CHECK(flow.phase() == Phase::Heist);
		events = flow.skip_phase();
		REQUIRE_FALSE(events.empty());
		CHECK(events.back().type == Type::Ended);
		CHECK(flow.outcome() == Outcome::TimeUp);
	}

	TEST_CASE("Las partes de cada fase se normalizan y los avisos fuera de rango se ignoran") {
		MatchFlow flow;
		MatchFlow::Rules rules;
		rules.duration = 30.0f;
		rules.preparation_share = 1.0f;
		rules.assault_share = 1.0f;
		rules.heist_share = 1.0f; // suman 3: un tercio cada una
		rules.warnings = { 60.0f, 5.0f }; // 60 >= duración: se ignora
		flow.start(rules);
		CHECK(flow.phase_start(Phase::Assault) == doctest::Approx(10.0f));
		CHECK(flow.phase_start(Phase::Heist) == doctest::Approx(20.0f));
		int warnings = 0;
		for (const auto &e : flow.advance(29.0f)) {
			warnings += e.type == Type::TimeWarning ? 1 : 0;
		}
		CHECK(warnings == 1);
	}
}
