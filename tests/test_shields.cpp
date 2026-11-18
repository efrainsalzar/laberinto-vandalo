#include <doctest/doctest.h>

#include "core/shields.h"

#include <memory>

using namespace lv::core;

namespace {
DamageInfo hit(DamageKind p_kind, float p_dir_x, float p_dir_y, float p_amount = 20.0f) {
	DamageInfo info;
	info.amount = p_amount;
	info.kind = p_kind;
	info.dir_x = p_dir_x;
	info.dir_y = p_dir_y;
	info.source = 1;
	return info;
}
const ShieldContext AIM_RIGHT = { 1.0f, 0.0f }; // mira hacia +x
} // namespace

TEST_SUITE("Escudos (cadena de modificadores)") {
	TEST_CASE("Sin escudos el golpe pasa") {
		ShieldChain chain;
		CHECK(chain.filter(hit(DamageKind::Bullet, 1, 0), AIM_RIGHT).outcome == ShieldOutcome::Pass);
	}

	TEST_CASE("La burbuja absorbe un golpe y se rompe") {
		ShieldChain chain;
		chain.add(std::make_unique<BubbleShield>(20.0f));
		const auto r = chain.filter(hit(DamageKind::Explosion, 0, 0, 80.0f), AIM_RIGHT);
		CHECK(r.outcome == ShieldOutcome::Absorbed);
		CHECK(r.by == ShieldKind::Bubble);
		std::vector<ShieldKind> ended;
		chain.remove_spent(ended);
		REQUIRE(ended.size() == 1);
		CHECK(ended[0] == ShieldKind::Bubble);
		CHECK(chain.filter(hit(DamageKind::Bullet, 1, 0), AIM_RIGHT).outcome == ShieldOutcome::Pass);
	}

	TEST_CASE("La burbuja aguanta medio segundo de láser (35) y luego se rompe") {
		BubbleShield bubble(20.0f, 35.0f);
		int frames = 0;
		while (!bubble.is_spent() && frames < 100) {
			CHECK(bubble.filter(hit(DamageKind::Beam, 1, 0, 70.0f / 60.0f), AIM_RIGHT) == ShieldOutcome::Absorbed);
			frames++;
		}
		CHECK(frames >= 30); // ~0,5 s (30 o 31 frames por redondeo)
		CHECK(frames <= 31);
	}

	TEST_CASE("El frontal para lo que viene de delante (±60°) y deja pasar lo de atrás y los costados") {
		FrontalShield frontal(8.0f, 60.0f);
		// Mira a +x. Una bala que viaja hacia -x viene desde delante.
		CHECK(frontal.filter(hit(DamageKind::Bullet, -1, 0), AIM_RIGHT) == ShieldOutcome::Absorbed);
		// Desde 45° (dentro del cono).
		CHECK(frontal.filter(hit(DamageKind::Bullet, -0.7071f, -0.7071f), AIM_RIGHT) == ShieldOutcome::Absorbed);
		// Desde el costado (90°): pasa.
		CHECK(frontal.filter(hit(DamageKind::Bullet, 0, 1), AIM_RIGHT) == ShieldOutcome::Pass);
		// Desde atrás: pasa.
		CHECK(frontal.filter(hit(DamageKind::Bullet, 1, 0), AIM_RIGHT) == ShieldOutcome::Pass);
		// No se gasta con los golpes.
		CHECK_FALSE(frontal.is_spent());
	}

	TEST_CASE("El frontal no para una explosión justo encima (sin dirección)") {
		FrontalShield frontal(8.0f, 60.0f);
		CHECK(frontal.filter(hit(DamageKind::Explosion, 0, 0), AIM_RIGHT) == ShieldOutcome::Pass);
		CHECK(frontal.filter(hit(DamageKind::Bullet, -1, 0), ShieldContext{}) == ShieldOutcome::Pass);
	}

	TEST_CASE("El espejo devuelve balas y láser, no explosiones, y no devuelve lo ya devuelto") {
		MirrorShield mirror(5.0f);
		CHECK(mirror.filter(hit(DamageKind::Bullet, 1, 0), AIM_RIGHT) == ShieldOutcome::Reflected);
		CHECK(mirror.filter(hit(DamageKind::Beam, 1, 0), AIM_RIGHT) == ShieldOutcome::Reflected);
		CHECK(mirror.filter(hit(DamageKind::Explosion, 1, 0), AIM_RIGHT) == ShieldOutcome::Pass);
		DamageInfo back = hit(DamageKind::Bullet, 1, 0);
		back.reflected = true;
		CHECK(mirror.filter(back, AIM_RIGHT) == ShieldOutcome::Absorbed); // espejo contra espejo: sin ping-pong
	}

	TEST_CASE("Orden fijo Burbuja → Frontal → Espejo: decide el primero que no deja pasar") {
		ShieldChain chain;
		chain.add(std::make_unique<MirrorShield>(5.0f));
		chain.add(std::make_unique<FrontalShield>(8.0f, 60.0f));
		chain.add(std::make_unique<BubbleShield>(20.0f));
		REQUIRE(chain.links().size() == 3);
		CHECK(chain.links()[0]->kind() == ShieldKind::Bubble);
		CHECK(chain.links()[1]->kind() == ShieldKind::Frontal);
		CHECK(chain.links()[2]->kind() == ShieldKind::Mirror);
		// 1.º la burbuja se lo come.
		CHECK(chain.filter(hit(DamageKind::Bullet, -1, 0), AIM_RIGHT).by == ShieldKind::Bubble);
		std::vector<ShieldKind> ended;
		chain.remove_spent(ended);
		// 2.º desde delante: frontal.
		CHECK(chain.filter(hit(DamageKind::Bullet, -1, 0), AIM_RIGHT).by == ShieldKind::Frontal);
		// 3.º desde atrás: el frontal deja pasar y el espejo lo devuelve.
		const auto r = chain.filter(hit(DamageKind::Bullet, 1, 0), AIM_RIGHT);
		CHECK(r.outcome == ShieldOutcome::Reflected);
		CHECK(r.by == ShieldKind::Mirror);
	}

	TEST_CASE("Mismo tipo no se apila: renueva el tiempo") {
		ShieldChain chain;
		chain.add(std::make_unique<MirrorShield>(5.0f));
		std::vector<ShieldKind> ended;
		chain.tick(4.0f, ended);
		CHECK(chain.find(ShieldKind::Mirror)->time_left() == doctest::Approx(1.0f));
		chain.add(std::make_unique<MirrorShield>(5.0f));
		CHECK(chain.links().size() == 1);
		CHECK(chain.find(ShieldKind::Mirror)->time_left() == doctest::Approx(5.0f));
	}

	TEST_CASE("Se acaban con el tiempo y avisan cuáles terminaron") {
		ShieldChain chain;
		chain.add(std::make_unique<FrontalShield>(8.0f, 60.0f));
		chain.add(std::make_unique<MirrorShield>(5.0f));
		std::vector<ShieldKind> ended;
		chain.tick(5.5f, ended);
		REQUIRE(ended.size() == 1);
		CHECK(ended[0] == ShieldKind::Mirror);
		ended.clear();
		chain.tick(3.0f, ended);
		REQUIRE(ended.size() == 1);
		CHECK(ended[0] == ShieldKind::Frontal);
		CHECK(chain.empty());
	}
}
