#pragma once

namespace lv::core {

// Datos de un arma (patrón Type Object: cada arma es un juego de valores, no una clase).
// En Godot vienen de un WeaponType (.tres). Distancias en casillas. Ver docs/ARQUITECTURA.md §22.
struct WeaponSpec {
	float fire_interval = 0.35f; // segundos mínimos entre disparos (cadencia)
	int magazine_size = 0; // balas por cargador; 0 = infinitas (la pistola inicial)
	float reload_time = 1.2f; // segundos para recargar al vaciar el cargador
	float projectile_speed = 14.0f; // casillas por segundo
	float range = 10.0f; // casillas que recorre la bala antes de desaparecer
	float projectile_radius = 0.15f; // radio de la bala (casillas)
};

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
	float time_left() const { return timer_; }
	const WeaponSpec &spec() const { return spec_; }

private:
	WeaponSpec spec_;
	State state_ = State::Ready;
	float timer_ = 0.0f;
	int ammo_ = -1;
};

} // namespace lv::core
