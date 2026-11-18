#pragma once

#include <vector>

namespace lv::core {

// Cómo dispara un arma (Strategy elegida por dato):
// - Projectile: balas que vuelan (pistola, escopeta, bazuca).
// - Beam: rayo instantáneo hasta la primera pared o el primer blanco (láser).
enum class FireMode {
	Projectile,
	Beam,
};

// Datos de un arma (patrón Type Object: cada arma es un juego de valores, no una clase).
// En Godot vienen de un WeaponType (.tres). Distancias en casillas. Ver docs/ARQUITECTURA.md §22.
struct WeaponSpec {
	float fire_interval = 0.35f; // segundos mínimos entre disparos (cadencia)
	int magazine_size = 0; // balas por cargador; 0 = infinitas (la pistola inicial)
	float reload_time = 1.2f; // segundos para recargar al vaciar el cargador
	float projectile_speed = 14.0f; // casillas por segundo
	float range = 10.0f; // casillas que recorre la bala antes de desaparecer
	float projectile_radius = 0.15f; // radio de la bala (casillas)
	// El alcance sale de la visión de quien dispara (no se dispara "a la nada", más allá de lo que se ve).
	bool range_from_vision = false;
	float range_vision_factor = 1.0f; // alcance = visión × factor

	FireMode fire_mode = FireMode::Projectile;
	int pellets = 1; // balas por disparo (escopeta: varias)
	float spread_deg = 0.0f; // apertura total del cono en grados (se reparten parejas, sin azar)
	float explosion_radius = 0.0f; // casillas; 0 = impacto directo. >0: explota al terminar (bazuca)
	int ammo = 0; // disparos en total; 0 = infinitos. Al gastarse, el arma queda vacía (armas temporales)
};

struct Direction {
	float x = 0.0f;
	float y = 0.0f;
};

// Direcciones de los `pellets` perdigones repartidas parejas en un cono de `spread_deg` centrado en (dir_x, dir_y).
// Normalizadas. Con 1 perdigón (o cono 0) todos salen derecho.
void spread_directions(float p_dir_x, float p_dir_y, int p_pellets, float p_spread_deg, std::vector<Direction> &r_out);

// Alcance real del arma para quien tiene visión p_vision (casillas).
float effective_range(const WeaponSpec &p_spec, float p_vision);

// Estado de un arma en uso (patrón State, como enum: pocos estados con temporizador).
//
//   Ready ──fire──► Cooling ──fire_interval──► Ready
//     └── (cargador vacío) ──► Reloading ──reload_time──► Ready (cargador lleno)
class WeaponState {
public:
	enum class State {
		Ready,
		Cooling,
		Reloading,
	};

	// Carga los datos y deja el arma lista con el cargador lleno.
	void setup(const WeaponSpec &p_spec);
	void tick(float p_delta);
	// Dispara si puede. Devuelve true si salió una bala.
	bool try_fire();

	State state() const { return state_; }
	bool is_ready() const { return state_ == State::Ready; }
	bool has_infinite_ammo() const { return spec_.magazine_size <= 0; }
	int ammo() const { return ammo_; } // balas en el cargador (-1 = infinitas)
	// Disparos que le quedan en total (-1 = infinitos). Con 0 el arma está vacía y ya no dispara.
	int total_ammo() const { return total_ammo_; }
	bool is_depleted() const { return total_ammo_ == 0; }
	float time_left() const { return timer_; }
	const WeaponSpec &spec() const { return spec_; }

private:
	WeaponSpec spec_;
	State state_ = State::Ready;
	float timer_ = 0.0f;
	int ammo_ = -1;
	int total_ammo_ = -1;
};

} // namespace lv::core
