#include <doctest/doctest.h>

#include "core/map_change_queue.h"
#include "core/random.h"
#include "core/spatial_hash.h"

#include <algorithm>
#include <cmath>

using lv::core::ChangePriority;
using lv::core::MapChangeOrder;
using lv::core::MapChangeQueue;
using lv::core::SpatialHash;

namespace {
MapChangeOrder order(int p_a, int p_b, bool p_open, ChangePriority p_priority, int p_source = 0) {
	return { { p_a, p_b, p_open }, p_priority, p_source };
}
} // namespace

TEST_SUITE("MapChangeQueue (prioridades, reservas, tope)") {
	TEST_CASE("Entrega por prioridad y en orden de llegada dentro de la misma prioridad") {
		MapChangeQueue q;
		q.push(order(1, 2, true, ChangePriority::Ambient));
		q.push(order(3, 4, true, ChangePriority::Player));
		q.push(order(5, 6, true, ChangePriority::Ambient));
		q.push(order(7, 8, true, ChangePriority::Director));
		const auto taken = q.take(10);
		REQUIRE(taken.size() == 4);
		CHECK(taken[0].change.a == 3);
		CHECK(taken[1].change.a == 7);
		CHECK(taken[2].change.a == 1);
		CHECK(taken[3].change.a == 5);
	}

	TEST_CASE("Presupuesto: take entrega como máximo N") {
		MapChangeQueue q;
		for (int i = 0; i < 10; i++) {
			q.push(order(i * 2, i * 2 + 1, true, ChangePriority::Ambient));
		}
		CHECK(q.take(3).size() == 3);
		CHECK(q.size() == 7);
	}

	TEST_CASE("Mismo pasaje: gana la de mayor prioridad; en empate, la que ya estaba") {
		MapChangeQueue q;
		CHECK(q.push(order(1, 2, false, ChangePriority::Ambient)));
		CHECK(q.push(order(2, 1, true, ChangePriority::Player))); // mismo pasaje (orden invertido)
		CHECK(q.size() == 1);
		CHECK_FALSE(q.push(order(1, 2, false, ChangePriority::Player))); // empate: se queda la anterior
		const auto taken = q.take(5);
		REQUIRE(taken.size() == 1);
		CHECK(taken[0].change.open);
		CHECK(taken[0].priority == ChangePriority::Player);
	}

	TEST_CASE("Reservas: bloquean a fuentes de igual o menor prioridad; una mayor las pisa") {
		MapChangeQueue q;
		CHECK(q.reserve(1, 2, /*source*/ 10, ChangePriority::Ambient));
		CHECK_FALSE(q.push(order(1, 2, true, ChangePriority::Ambient, 20))); // otra fuente, misma prioridad
		CHECK(q.push(order(1, 2, true, ChangePriority::Ambient, 10))); // la dueña sí puede
		q.clear();
		CHECK(q.reserve(1, 2, 10, ChangePriority::Director));
		CHECK_FALSE(q.reserve(1, 2, 20, ChangePriority::Ambient));
		CHECK(q.push(order(1, 2, false, ChangePriority::Player, 30))); // mayor prioridad: pisa
		CHECK_FALSE(q.is_reserved_by(1, 2, 10)); // la fuente 10 perdió su reserva
		q.release(1, 2, 30);
	}

	TEST_CASE("Tope: se descarta la de menor prioridad") {
		MapChangeQueue q(3);
		q.push(order(1, 2, true, ChangePriority::Director));
		q.push(order(3, 4, true, ChangePriority::Ambient));
		q.push(order(5, 6, true, ChangePriority::Director));
		CHECK(q.push(order(7, 8, true, ChangePriority::Player))); // entra; sale la Ambient
		CHECK(q.size() == 3);
		CHECK_FALSE(q.push(order(9, 10, true, ChangePriority::Ambient))); // no entra: es la peor
		for (const auto &o : q.take(10)) {
			CHECK(o.priority != ChangePriority::Ambient);
		}
		CHECK(q.dropped_count() == 2);
	}
}

TEST_SUITE("SpatialHash") {
	TEST_CASE("La consulta por radio coincide con revisar todo (fuerza bruta)") {
		SpatialHash hash(32.0f);
		lv::core::Rng rng(3);
		std::vector<std::pair<float, float>> pos(300);
		for (int i = 0; i < 300; i++) {
			pos[i] = { rng.unit() * 2000.0f - 500.0f, rng.unit() * 1500.0f - 300.0f };
			hash.insert(i, pos[i].first, pos[i].second);
		}
		// Mover la mitad.
		for (int i = 0; i < 300; i += 2) {
			pos[i] = { pos[i].first + 77.0f, pos[i].second - 31.0f };
			hash.move(i, pos[i].first, pos[i].second);
		}
		hash.remove(5);
		for (int q = 0; q < 40; q++) {
			const float x = rng.unit() * 1800.0f - 400.0f;
			const float y = rng.unit() * 1300.0f - 200.0f;
			const float r = 20.0f + rng.unit() * 200.0f;
			std::vector<int> fast;
			hash.query_radius(x, y, r, fast);
			std::vector<int> brute;
			for (int i = 0; i < 300; i++) {
				const float dx = pos[i].first - x;
				const float dy = pos[i].second - y;
				if (i != 5 && dx * dx + dy * dy <= r * r) {
					brute.push_back(i);
				}
			}
			std::sort(fast.begin(), fast.end());
			CHECK(fast == brute);
		}
		CHECK(hash.size() == 299);
	}

	TEST_CASE("Insertar dos veces el mismo id lo mueve, no lo duplica") {
		SpatialHash hash(10.0f);
		hash.insert(1, 0, 0);
		hash.insert(1, 100, 100);
		std::vector<int> near_origin;
		hash.query_radius(0, 0, 5, near_origin);
		CHECK(near_origin.empty());
		CHECK(hash.size() == 1);
	}
}
