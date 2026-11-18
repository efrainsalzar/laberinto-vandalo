#include <doctest/doctest.h>

#include "core/bot_gunner.h"
#include "core/line_of_sight.h"
#include "core/projectiles.h"
#include "core/weapon.h"

#include <cmath>

using lv::core::BotGunner;
using lv::core::GridPoint;
using lv::core::GunDecision;
using lv::core::GunTarget;
using lv::core::MazeGrid;
using lv::core::ProjectileEvent;
using lv::core::ProjectileTarget;
using lv::core::ProjectileWorld;
using lv::core::Rng;
using lv::core::WeaponSpec;
using lv::core::WeaponState;

namespace {
// Bóveda grande: sala abierta en el centro (de 4 a 20) y paredes alrededor (el borde está en 0 y 24).
MazeGrid open_room() {
	MazeGrid grid;
	MazeGrid::Options options;
	options.cells_w = 6;
	options.cells_h = 6;
	options.corridor_width = 3;
	options.vault_radius = 8;
	options.seed = 1;
	grid.generate(options);
	return grid;
}

WeaponSpec pistol() {
	WeaponSpec spec;
	spec.fire_interval = 0.5f;
	spec.magazine_size = 0;
	spec.projectile_speed = 10.0f;
	spec.range = 6.0f;
	spec.projectile_radius = 0.1f;
	return spec;
}

const std::vector<ProjectileTarget> NO_TARGETS;
} // namespace

TEST_SUITE("Armas") {
	TEST_CASE("La cadencia impide disparar antes de tiempo") {
		WeaponState weapon;
		weapon.setup(pistol());
		CHECK(weapon.try_fire());
		CHECK_FALSE(weapon.try_fire());
		weapon.tick(0.3f);
		CHECK_FALSE(weapon.try_fire());
		weapon.tick(0.25f);
		CHECK(weapon.is_ready());
		CHECK(weapon.try_fire());
	}

	TEST_CASE("La pistola tiene munición infinita") {
		WeaponState weapon;
		weapon.setup(pistol());
		CHECK(weapon.has_infinite_ammo());
		for (int i = 0; i < 500; i++) {
			REQUIRE(weapon.try_fire());
			weapon.tick(0.5f);
		}
		CHECK(weapon.ammo() == -1);
	}

	TEST_CASE("Con cargador: al vaciarlo recarga y vuelve lleno") {
		WeaponSpec spec = pistol();
		spec.magazine_size = 2;
		spec.reload_time = 1.0f;
		WeaponState weapon;
		weapon.setup(spec);
		CHECK(weapon.try_fire());
		weapon.tick(0.5f);
		CHECK(weapon.try_fire());
		CHECK(weapon.state() == WeaponState::State::Reloading);
		weapon.tick(0.6f);
		CHECK_FALSE(weapon.try_fire());
		weapon.tick(0.5f);
		CHECK(weapon.ammo() == 2);
		CHECK(weapon.try_fire());
	}
}

TEST_SUITE("Balas") {
	TEST_CASE("La bala avanza y desaparece al llegar a su alcance") {
		const MazeGrid grid = open_room();
		ProjectileWorld world;
		const auto c = grid.center();
		REQUIRE(world.spawn(c.x + 0.5f, c.y + 0.5f, 0.0f, 1.0f, pistol(), 0));
		std::vector<ProjectileEvent> events;
		world.step(0.1f, grid, NO_TARGETS, events); // 1 casilla
		REQUIRE(world.size() == 1);
		CHECK(world.projectiles()[0].y == doctest::Approx(c.y + 1.5f));
		CHECK(events.empty());
		for (int i = 0; i < 10; i++) {
			world.step(0.1f, grid, NO_TARGETS, events);
		}
		CHECK(world.size() == 0);
		REQUIRE(events.size() == 1);
		CHECK(events[0].kind == ProjectileEvent::Kind::Expired);
		CHECK(events[0].y == doctest::Approx(c.y + 6.5f)); // recorrió exactamente su alcance
	}

	TEST_CASE("Una bala muy rápida no atraviesa las paredes") {
		const MazeGrid grid = open_room();
		ProjectileWorld world;
		WeaponSpec spec = pistol();
		spec.projectile_speed = 500.0f;
		spec.range = 100.0f;
		const auto c = grid.center();
		REQUIRE(world.spawn(c.x + 0.5f, c.y + 0.5f, 1.0f, 0.0f, spec, 0));
		std::vector<ProjectileEvent> events;
		world.step(1.0f, grid, NO_TARGETS, events); // en un solo frame recorrería 100 casillas
		REQUIRE(events.size() == 1);
		CHECK(events[0].kind == ProjectileEvent::Kind::HitWall);
		CHECK(events[0].x <= grid.width()); // se quedó dentro del mapa
		const int tile_x = static_cast<int>(std::floor(events[0].x + 0.1f)); // el choque queda 0,05 antes del borde
		CHECK(grid.is_wall({ tile_x, c.y })); // terminó justo en el borde de una pared
		CHECK_FALSE(grid.is_wall({ tile_x - 1, c.y }));
	}

	TEST_CASE("segment_hits_wall da la fracción del primer choque") {
		const MazeGrid grid = open_room();
		const auto c = grid.center();
		float t = -1.0f;
		CHECK_FALSE(lv::core::segment_hits_wall(grid, { c.x + 0.5f, c.y + 0.5f }, { c.x + 3.5f, c.y + 0.5f }, t));
		CHECK(lv::core::segment_hits_wall(grid, { c.x + 0.5f, c.y + 0.5f }, { c.x + 0.5f, -5.0f }, t));
		CHECK(t > 0.0f);
		CHECK(t < 1.0f);
	}

	TEST_CASE("Le pega a un blanco en su camino, pero nunca a quien disparó") {
		const MazeGrid grid = open_room();
		ProjectileWorld world;
		const auto c = grid.center();
		const float x = c.x + 0.5f;
		const float y = c.y + 0.5f;
		std::vector<ProjectileTarget> targets = { { 0, x, y, 0.4f }, { 7, x + 3.0f, y, 0.4f } };
		REQUIRE(world.spawn(x, y, 1.0f, 0.0f, pistol(), 0)); // sale desde dentro del tirador
		std::vector<ProjectileEvent> events;
		for (int i = 0; i < 5 && events.empty(); i++) {
			world.step(0.1f, grid, targets, events);
		}
		REQUIRE(events.size() == 1);
		CHECK(events[0].kind == ProjectileEvent::Kind::HitTarget);
		CHECK(events[0].target == 7);
		CHECK(events[0].owner == 0);
		CHECK(events[0].x == doctest::Approx(x + 3.0f - 0.5f)); // borde del blanco (0,4 + 0,1 de la bala)
		CHECK(world.size() == 0);
	}

	TEST_CASE("Un blanco detrás de una pared está a salvo") {
		const MazeGrid grid = open_room();
		ProjectileWorld world;
		WeaponSpec spec = pistol();
		spec.range = 40.0f;
		spec.projectile_speed = 40.0f;
		const auto c = grid.center();
		const float y = c.y + 0.5f;
		// El borde del mapa (x = 0) es pared: un blanco "afuera" no se alcanza.
		std::vector<ProjectileTarget> targets = { { 3, -2.0f, y, 0.4f } };
		REQUIRE(world.spawn(c.x + 0.5f, y, -1.0f, 0.0f, spec, 0));
		std::vector<ProjectileEvent> events;
		world.step(1.0f, grid, targets, events);
		REQUIRE(events.size() == 1);
		CHECK(events[0].kind == ProjectileEvent::Kind::HitWall);
	}

	TEST_CASE("Al llegar al tope, la bala nueva reemplaza a la más vieja") {
		const MazeGrid grid = open_room();
		ProjectileWorld world(3);
		const auto c = grid.center();
		for (int i = 0; i < 4; i++) {
			REQUIRE(world.spawn(c.x + 0.5f, c.y + 0.5f, 1.0f, 0.0f, pistol(), 0, i));
		}
		CHECK(world.size() == 3);
		bool has_first = false;
		bool has_last = false;
		for (const auto &p : world.projectiles()) {
			has_first = has_first || p.tag == 0;
			has_last = has_last || p.tag == 3;
		}
		CHECK_FALSE(has_first);
		CHECK(has_last);
	}

	TEST_CASE("Sin dirección no se dispara") {
		ProjectileWorld world;
		CHECK_FALSE(world.spawn(1.0f, 1.0f, 0.0f, 0.0f, pistol(), 0));
		CHECK(world.size() == 0);
	}
}

TEST_SUITE("Puntería de los bots") {
	TEST_CASE("Sin rivales visibles no dispara") {
		BotGunner gunner;
		gunner.setup({ 0.0f, 0.0f, 10.0f });
		Rng rng(1);
		const GunDecision d = gunner.update(0.1f, 0.0f, 0.0f, {}, rng);
		CHECK_FALSE(d.fire);
		CHECK(d.target == -1);
	}

	TEST_CASE("Espera su tiempo de reacción antes del primer disparo") {
		BotGunner gunner;
		gunner.setup({ 0.5f, 0.0f, 10.0f });
		Rng rng(1);
		const std::vector<GunTarget> targets = { { 4, 3.0f, 0.0f } };
		CHECK_FALSE(gunner.update(0.2f, 0.0f, 0.0f, targets, rng).fire);
		CHECK_FALSE(gunner.update(0.2f, 0.0f, 0.0f, targets, rng).fire);
		CHECK_FALSE(gunner.update(0.2f, 0.0f, 0.0f, targets, rng).fire);
		const GunDecision d = gunner.update(0.2f, 0.0f, 0.0f, targets, rng);
		CHECK(d.fire);
		CHECK(d.target == 4);
		CHECK(d.aim_x == doctest::Approx(3.0f)); // sin error de puntería: directo al blanco
		CHECK(d.aim_y == doctest::Approx(0.0f));
	}

	TEST_CASE("Elige al más cercano y no dispara fuera de alcance") {
		BotGunner gunner;
		gunner.setup({ 0.0f, 0.0f, 5.0f });
		Rng rng(1);
		const std::vector<GunTarget> targets = { { 1, 4.0f, 0.0f }, { 2, 0.0f, 2.0f }, { 3, 9.0f, 0.0f } };
		CHECK(gunner.update(0.1f, 0.0f, 0.0f, targets, rng).target == 2);
		const std::vector<GunTarget> far = { { 3, 9.0f, 0.0f } };
		CHECK_FALSE(gunner.update(0.1f, 0.0f, 0.0f, far, rng).fire);
	}

	TEST_CASE("El error de puntería respeta su límite y es reproducible con la semilla") {
		BotGunner a;
		BotGunner b;
		a.setup({ 0.0f, 10.0f, 20.0f });
		b.setup({ 0.0f, 10.0f, 20.0f });
		Rng rng_a(42);
		Rng rng_b(42);
		const std::vector<GunTarget> targets = { { 1, 5.0f, 0.0f } };
		const float max_angle = 10.0f * 3.14159265f / 180.0f + 1e-4f;
		bool some_error = false;
		for (int i = 0; i < 50; i++) {
			const GunDecision da = a.update(0.1f, 0.0f, 0.0f, targets, rng_a);
			const GunDecision db = b.update(0.1f, 0.0f, 0.0f, targets, rng_b);
			REQUIRE(da.fire);
			CHECK(da.aim_x == db.aim_x);
			CHECK(da.aim_y == db.aim_y);
			const float angle = std::atan2(da.aim_y, da.aim_x);
			CHECK(std::abs(angle) <= max_angle);
			some_error = some_error || std::abs(angle) > 0.01f;
		}
		CHECK(some_error);
	}
}

TEST_SUITE("Alcance") {
	TEST_CASE("Con alcance por visión, el arma llega hasta donde se ve") {
		WeaponSpec spec = pistol();
		CHECK(lv::core::effective_range(spec, 5.0f) == doctest::Approx(6.0f)); // alcance propio
		spec.range_from_vision = true;
		CHECK(lv::core::effective_range(spec, 5.0f) == doctest::Approx(5.0f));
		CHECK(lv::core::effective_range(spec, 7.5f) == doctest::Approx(7.5f)); // con power-up de visión
		spec.range_vision_factor = 0.5f;
		CHECK(lv::core::effective_range(spec, 5.0f) == doctest::Approx(2.5f));
	}

	TEST_CASE("La bala lleva la cuenta de lo recorrido") {
		const MazeGrid grid = open_room();
		ProjectileWorld world;
		const auto c = grid.center();
		REQUIRE(world.spawn(c.x + 0.5f, c.y + 0.5f, 0.0f, 1.0f, pistol(), 0));
		std::vector<ProjectileEvent> events;
		world.step(0.2f, grid, NO_TARGETS, events);
		CHECK(world.projectiles()[0].traveled == doctest::Approx(2.0f));
	}
}

TEST_SUITE("Catálogo de armas") {
	TEST_CASE("Escopeta: perdigones repartidos parejos dentro del cono") {
		std::vector<lv::core::Direction> dirs;
		lv::core::spread_directions(1.0f, 0.0f, 5, 40.0f, dirs);
		REQUIRE(dirs.size() == 5);
		const float half = 20.0f * 3.14159265f / 180.0f;
		CHECK(std::atan2(dirs[0].y, dirs[0].x) == doctest::Approx(-half));
		CHECK(std::atan2(dirs[2].y, dirs[2].x) == doctest::Approx(0.0f));
		CHECK(std::atan2(dirs[4].y, dirs[4].x) == doctest::Approx(half));
		for (const auto &d : dirs) {
			CHECK(d.x * d.x + d.y * d.y == doctest::Approx(1.0f));
		}
		lv::core::spread_directions(0.0f, 3.0f, 1, 40.0f, dirs); // un perdigón: derecho
		REQUIRE(dirs.size() == 1);
		CHECK(dirs[0].y == doctest::Approx(1.0f));
	}

	TEST_CASE("Arma temporal: al gastar su munición queda vacía") {
		WeaponSpec spec = pistol();
		spec.ammo = 3;
		spec.fire_interval = 0.0f;
		WeaponState weapon;
		weapon.setup(spec);
		CHECK(weapon.total_ammo() == 3);
		CHECK(weapon.try_fire());
		CHECK(weapon.try_fire());
		CHECK(weapon.try_fire());
		CHECK(weapon.is_depleted());
		CHECK_FALSE(weapon.try_fire());
		WeaponState pistol_state;
		pistol_state.setup(pistol());
		CHECK(pistol_state.total_ammo() == -1); // la pistola nunca se vacía
		CHECK_FALSE(pistol_state.is_depleted());
	}

	TEST_CASE("Láser: el rayo le pega al primer blanco, nunca al tirador, y lo frenan las paredes") {
		const MazeGrid grid = open_room();
		const auto c = grid.center();
		const float x = c.x + 0.5f;
		const float y = c.y + 0.5f;
		const std::vector<ProjectileTarget> targets = { { 0, x, y, 0.4f }, { 5, x + 4.0f, y, 0.4f }, { 6, x + 2.0f, y, 0.4f } };
		const lv::core::BeamHit hit = lv::core::trace_beam(grid, x, y, 1.0f, 0.0f, 8.0f, targets, 0);
		CHECK(hit.target == 6);
		CHECK(hit.end_x == doctest::Approx(x + 1.6f));
		const lv::core::BeamHit miss = lv::core::trace_beam(grid, x, y, 0.0f, 1.0f, 3.0f, targets, 0);
		CHECK(miss.target == -1);
		CHECK(miss.end_y == doctest::Approx(y + 3.0f)); // llega a su alcance
		const lv::core::BeamHit wall = lv::core::trace_beam(grid, x, y, -1.0f, 0.0f, 100.0f, targets, 0);
		CHECK(wall.target == -1);
		CHECK(wall.end_x > 0.0f); // se detuvo en una pared, no salió del mapa
	}

	TEST_CASE("Bazuca: la bala explota y la explosión respeta radio, paredes y tirador") {
		const MazeGrid grid = open_room();
		const auto c = grid.center();
		const float x = c.x + 0.5f;
		const float y = c.y + 0.5f;
		WeaponSpec spec = pistol();
		spec.explosion_radius = 2.0f;
		ProjectileWorld world;
		REQUIRE(world.spawn(x, y, 1.0f, 0.0f, spec, 0));
		std::vector<ProjectileEvent> events;
		for (int i = 0; i < 10 && events.empty(); i++) {
			world.step(0.1f, grid, NO_TARGETS, events);
		}
		REQUIRE(events.size() == 1);
		CHECK(events[0].explosion_radius == doctest::Approx(2.0f));

		const std::vector<ProjectileTarget> targets = {
			{ 0, x - 1.0f, y, 0.4f }, // el tirador, cerca: no le pasa nada
			{ 1, x + 1.5f, y, 0.4f }, // dentro del radio
			{ 2, x, y + 2.3f, 0.4f }, // justo en el borde (radio + cuerpo)
			{ 3, x + 4.0f, y, 0.4f }, // lejos
			{ 4, -2.0f, y, 0.4f }, // fuera del mapa, detrás de la pared del borde
		};
		std::vector<lv::core::ExplosionVictim> victims;
		lv::core::explosion_victims(grid, x, y, 2.0f, targets, 0, victims);
		REQUIRE(victims.size() == 2);
		CHECK(victims[0].id == 1);
		CHECK(victims[0].distance == doctest::Approx(1.1f)); // 1,5 al centro − 0,4 de cuerpo
		CHECK(victims[1].id == 2);
		CHECK(victims[1].distance == doctest::Approx(1.9f));
	}

	TEST_CASE("El choque contra una pared queda sobre el suelo (la explosión no nace dentro de la pared)") {
		const MazeGrid grid = open_room();
		const auto c = grid.center();
		WeaponSpec spec = pistol();
		spec.range = 100.0f;
		spec.projectile_speed = 100.0f;
		ProjectileWorld world;
		REQUIRE(world.spawn(c.x + 0.5f, c.y + 0.5f, 1.0f, 0.0f, spec, 0));
		std::vector<ProjectileEvent> events;
		world.step(1.0f, grid, NO_TARGETS, events);
		REQUIRE(events.size() == 1);
		REQUIRE(events[0].kind == ProjectileEvent::Kind::HitWall);
		CHECK_FALSE(grid.is_wall({ static_cast<int>(std::floor(events[0].x)), static_cast<int>(std::floor(events[0].y)) }));
	}
}

TEST_SUITE("Disparo sostenido") {
	TEST_CASE("Estado del gatillo: apretar, mantener, soltar") {
		using lv::core::Trigger;
		CHECK(lv::core::next_trigger(false, false) == Trigger::Idle);
		CHECK(lv::core::next_trigger(false, true) == Trigger::Pressed);
		CHECK(lv::core::next_trigger(true, true) == Trigger::Held);
		CHECK(lv::core::next_trigger(true, false) == Trigger::Released);
		CHECK(lv::core::is_trigger_down(Trigger::Pressed));
		CHECK(lv::core::is_trigger_down(Trigger::Held));
		CHECK_FALSE(lv::core::is_trigger_down(Trigger::Released));
	}

	TEST_CASE("Batería: se gasta solo mientras se dispara y al acabarse queda vacía") {
		WeaponSpec spec;
		spec.fire_mode = lv::core::FireMode::HeldBeam;
		spec.energy = 1.0f;
		WeaponState laser;
		laser.setup(spec);
		CHECK(laser.energy() == doctest::Approx(1.0f));
		for (int i = 0; i < 6; i++) {
			REQUIRE(laser.drain(0.1f));
		}
		laser.tick(5.0f); // el tiempo sin disparar no gasta batería
		CHECK(laser.energy() == doctest::Approx(0.4f));
		for (int i = 0; i < 4; i++) {
			laser.drain(0.1f);
		}
		CHECK(laser.is_depleted());
		CHECK_FALSE(laser.drain(0.1f));
		WeaponState infinite;
		infinite.setup(WeaponSpec());
		CHECK(infinite.drain(100.0f)); // sin batería configurada: infinita
		CHECK(infinite.energy() < 0.0f);
	}

	TEST_CASE("Bot: el error de puntería se mantiene un rato (un rayo no tiembla en cada frame)") {
		BotGunner gunner;
		BotGunner::Tuning tuning;
		tuning.reaction_time = 0.0f;
		tuning.aim_error_deg = 20.0f;
		tuning.max_range = 20.0f;
		tuning.error_hold = 0.25f;
		gunner.setup(tuning);
		Rng rng(7);
		const std::vector<GunTarget> targets = { { 1, 5.0f, 0.0f } };
		const GunDecision a = gunner.update(0.05f, 0.0f, 0.0f, targets, rng);
		const GunDecision b = gunner.update(0.05f, 0.0f, 0.0f, targets, rng);
		CHECK(a.aim_y == doctest::Approx(b.aim_y)); // mismo error dentro de error_hold
	}
}
