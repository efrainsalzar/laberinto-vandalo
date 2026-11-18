#pragma once

namespace lv::core {

// Tipo de daño (para que los escudos de la pieza 3 y la vista puedan distinguirlo).
enum class DamageKind {
	Bullet, // bala o perdigón
	Explosion, // bazuca (y luego bombas y minas)
	Beam, // láser (llega en trocitos cada frame)
};

// Un golpe como dato (Value Object): viaja del arma a la salud pasando por los filtros (escudos, pieza 3).
// Simple y copiable: sirve igual para red y repeticiones. Posiciones y direcciones en casillas.
struct DamageInfo {
	float amount = 0.0f;
	int source = -1; // id del participante que lo causó (-1 = nadie / el mapa)
	DamageKind kind = DamageKind::Bullet;
	float dir_x = 0.0f; // hacia dónde iba el golpe (normalizada; 0,0 = sin dirección, p. ej. explosión encima)
	float dir_y = 0.0f;
	float x = 0.0f; // dónde pegó
	float y = 0.0f;
};

// Qué pasó al aplicar un golpe.
struct DamageResult {
	float applied = 0.0f; // salud que realmente se quitó (nunca más de la que había)
	bool killed = false; // este golpe la dejó en 0 (solo una vez: los golpes siguientes no vuelven a "matar")
};

// Salud de un participante: un número entre 0 y max. Lógica pura, sin tiempos.
// - damage(): quita salud; con 0 ya no recibe más daño hasta reset() (reaparecer).
// - heal(): suma sin pasar del máximo (pociones, pieza 2). Con 0 no cura (ya perdió la vida).
// Ver docs/ARQUITECTURA.md §25.
class Health {
public:
	explicit Health(float p_max = 100.0f) { setup(p_max); }

	// Cambia el máximo y deja la salud llena.
	void setup(float p_max);
	// Vuelve a la salud máxima (al reaparecer).
	void reset() { current_ = max_; }

	DamageResult damage(const DamageInfo &p_info);
	// Devuelve cuánto curó de verdad.
	float heal(float p_amount);

	float current() const { return current_; }
	float max() const { return max_; }
	float ratio() const { return max_ > 0.0f ? current_ / max_ : 0.0f; }
	bool is_dead() const { return current_ <= 0.0f; }

private:
	float max_ = 100.0f;
	float current_ = 100.0f;
};

// Daño de una explosión según qué tan lejos del centro quedó el cuerpo: `p_center` en el centro, bajando en línea
// recta hasta `p_edge` en el borde del radio. `p_distance` = del centro al borde del cuerpo (0 si lo toca).
float explosion_damage(float p_center, float p_edge, float p_radius, float p_distance);

// Agrupa daño que llega en trocitos (láser: un poco cada frame) para mostrarlo cada `interval` s como un solo número.
// - add(): suma un trocito. Si es el primero de un grupo, empieza a contar.
// - tick(): devuelve el total del grupo cuando se cumple el intervalo (y lo vacía); si no, 0.
// - flush(): devuelve lo pendiente ya (p. ej. al morir) y lo vacía.
class DamageBatch {
public:
	explicit DamageBatch(float p_interval = 0.25f) : interval_(p_interval) {}

	void add(float p_amount);
	float tick(float p_delta);
	float flush();
	bool has_pending() const { return pending_ > 0.0f; }
	float pending() const { return pending_; }

private:
	float interval_ = 0.25f;
	float pending_ = 0.0f;
	float timer_ = 0.0f;
};

} // namespace lv::core
