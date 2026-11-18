#include <doctest/doctest.h>

#include "core/health.h"

using lv::core::DamageBatch;
using lv::core::DamageInfo;
using lv::core::DamageKind;
using lv::core::Health;

namespace {
DamageInfo hit(float p_amount, DamageKind p_kind = DamageKind::Bullet) {
	DamageInfo info;
	info.amount = p_amount;
	info.kind = p_kind;
	info.source = 1;
	return info;
}
} // namespace

TEST_SUITE("Health (salud y daño)") {
	TEST_CASE("Empieza llena y cinco balas de 20 la dejan en 0") {
		Health health(100.0f);
		CHECK(health.current() == doctest::Approx(100.0f));
		CHECK(health.ratio() == doctest::Approx(1.0f));
		for (int i = 0; i < 4; i++) {
			const auto r = health.damage(hit(20.0f));
			CHECK(r.applied == doctest::Approx(20.0f));
			CHECK_FALSE(r.killed);
		}
		CHECK(health.current() == doctest::Approx(20.0f));
		const auto last = health.damage(hit(20.0f));
		CHECK(last.killed);
		CHECK(health.is_dead());
	}

	TEST_CASE("Nunca quita más de lo que hay y solo 'mata' una vez") {
		Health health(100.0f);
		health.damage(hit(90.0f));
		const auto r = health.damage(hit(80.0f));
		CHECK(r.applied == doctest::Approx(10.0f));
		CHECK(r.killed);
		CHECK(health.current() == doctest::Approx(0.0f));
		const auto again = health.damage(hit(50.0f));
		CHECK(again.applied == doctest::Approx(0.0f));
		CHECK_FALSE(again.killed);
	}

	TEST_CASE("Daño cero o negativo no hace nada") {
		Health health(100.0f);
		CHECK(health.damage(hit(0.0f)).applied == doctest::Approx(0.0f));
		CHECK(health.damage(hit(-10.0f)).applied == doctest::Approx(0.0f));
		CHECK(health.current() == doctest::Approx(100.0f));
	}

	TEST_CASE("El láser en trocitos (70/s) suma lo mismo que de una vez") {
		Health health(100.0f);
		for (int i = 0; i < 60; i++) { // 1 s a 60 frames
			health.damage(hit(70.0f / 60.0f, DamageKind::Beam));
		}
		CHECK(health.current() == doctest::Approx(30.0f).epsilon(0.001));
	}

	TEST_CASE("Curar no pasa del máximo y no revive") {
		Health health(100.0f);
		health.damage(hit(40.0f));
		CHECK(health.heal(25.0f) == doctest::Approx(25.0f));
		CHECK(health.heal(25.0f) == doctest::Approx(15.0f));
		CHECK(health.current() == doctest::Approx(100.0f));
		health.damage(hit(200.0f));
		CHECK(health.heal(50.0f) == doctest::Approx(0.0f));
		CHECK(health.is_dead());
	}

	TEST_CASE("reset() vuelve a la salud máxima (reaparecer)") {
		Health health(100.0f);
		health.damage(hit(150.0f));
		health.reset();
		CHECK(health.current() == doctest::Approx(100.0f));
		CHECK_FALSE(health.is_dead());
		CHECK(health.damage(hit(100.0f)).killed); // puede volver a perder la vida
	}

	TEST_CASE("Un máximo inválido no deja la salud en 0 (no nace muerto)") {
		Health health(0.0f);
		CHECK_FALSE(health.is_dead());
		CHECK(health.max() > 0.0f);
	}
}

TEST_SUITE("Daño de explosión") {
	TEST_CASE("80 en el centro, 30 en el borde, lineal en medio") {
		CHECK(lv::core::explosion_damage(80.0f, 30.0f, 2.0f, 0.0f) == doctest::Approx(80.0f));
		CHECK(lv::core::explosion_damage(80.0f, 30.0f, 2.0f, 1.0f) == doctest::Approx(55.0f));
		CHECK(lv::core::explosion_damage(80.0f, 30.0f, 2.0f, 2.0f) == doctest::Approx(30.0f));
	}

	TEST_CASE("Fuera de rango se limita a los extremos; radio 0 = daño del centro") {
		CHECK(lv::core::explosion_damage(80.0f, 30.0f, 2.0f, 5.0f) == doctest::Approx(30.0f));
		CHECK(lv::core::explosion_damage(80.0f, 30.0f, 2.0f, -1.0f) == doctest::Approx(80.0f));
		CHECK(lv::core::explosion_damage(80.0f, 30.0f, 0.0f, 1.0f) == doctest::Approx(80.0f));
	}
}

TEST_SUITE("DamageBatch (números del láser agrupados)") {
	TEST_CASE("Suma los trocitos y los entrega juntos cada 0,25 s") {
		DamageBatch batch(0.25f);
		float shown = 0.0f;
		int numbers = 0;
		for (int i = 0; i < 60; i++) { // 1 s de láser a 60 frames
			batch.add(70.0f / 60.0f);
			const float out = batch.tick(1.0f / 60.0f);
			if (out > 0.0f) {
				shown += out;
				numbers++;
			}
		}
		shown += batch.flush();
		CHECK(shown == doctest::Approx(70.0f).epsilon(0.001));
		CHECK(numbers >= 3);
		CHECK(numbers <= 4);
	}

	TEST_CASE("Sin daño pendiente no entrega nada") {
		DamageBatch batch(0.25f);
		CHECK(batch.tick(1.0f) == doctest::Approx(0.0f));
		CHECK_FALSE(batch.has_pending());
	}

	TEST_CASE("flush() entrega lo pendiente en el acto (al morir)") {
		DamageBatch batch(0.25f);
		batch.add(5.0f);
		CHECK(batch.tick(0.1f) == doctest::Approx(0.0f));
		CHECK(batch.flush() == doctest::Approx(5.0f));
		CHECK_FALSE(batch.has_pending());
	}

	TEST_CASE("Un grupo nuevo empieza a contar desde su primer trocito") {
		DamageBatch batch(0.25f);
		batch.add(1.0f);
		CHECK(batch.tick(0.3f) == doctest::Approx(1.0f));
		CHECK(batch.tick(1.0f) == doctest::Approx(0.0f)); // nada pendiente: el tiempo no se acumula
		batch.add(2.0f);
		CHECK(batch.tick(0.1f) == doctest::Approx(0.0f));
		CHECK(batch.tick(0.2f) == doctest::Approx(2.0f));
	}
}
