#pragma once

#include "core/weapon.h"
#include "game/weapon_type.h"

#include <godot_cpp/variant/vector2.hpp>

#include <memory>
#include <vector>

using namespace godot;

class Participant;
class ProjectileSystem;

// Lo que un comportamiento de disparo necesita saber en este frame.
struct FireContext {
	Participant *shooter = nullptr;
	ProjectileSystem *projectiles = nullptr;
	Ref<WeaponType> weapon;
	lv::core::WeaponState *state = nullptr; // cadencia, munición y batería del arma
	Vector2 from; // posición del tirador (mundo)
	Vector2 direction; // normalizada
	float range = 0.0f; // alcance real (casillas)
	float delta = 0.0f;
};

// Patrón Strategy: CÓMO dispara un arma. Recibe cada frame el estado del gatillo (apretado, mantenido, soltado)
// y decide qué hacer. WeaponComponent no sabe cómo dispara cada arma: solo le pasa el gatillo a su comportamiento.
// Un arma con un comportamiento nuevo (cargar el disparo, ráfaga, cuerpo a cuerpo...) = una clase nueva aquí +
// un valor de FireMode; nada más cambia. El .tres elige el comportamiento (WeaponType.fire_mode).
// Ver docs/ARQUITECTURA.md §23.
class FireBehavior {
public:
	virtual ~FireBehavior() = default;
	// Devuelve true si disparó en este frame.
	virtual bool update(const FireContext &p_context, lv::core::Trigger p_trigger) = 0;

	// Factory: el comportamiento de un modo de disparo.
	static std::unique_ptr<FireBehavior> create(lv::core::FireMode p_mode);
};

// Balas que vuelan, a la cadencia del arma, mientras el gatillo está abajo (pistola, escopeta, bazuca).
// Varios perdigones por disparo si el arma los tiene (cono parejo).
class ProjectileFire : public FireBehavior {
public:
	bool update(const FireContext &p_context, lv::core::Trigger p_trigger) override;

private:
	std::vector<lv::core::Direction> directions_;
};

// Un rayo continuo mientras se mantiene el gatillo: sigue la puntería cada frame y gasta batería (láser).
class HeldBeamFire : public FireBehavior {
public:
	bool update(const FireContext &p_context, lv::core::Trigger p_trigger) override;
};
