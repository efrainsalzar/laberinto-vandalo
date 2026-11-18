#pragma once

#include "core/health.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/vector2.hpp>

using namespace godot;

class Participant;

// Salud de un participante, como componente hijo (composición, junto a LifeComponent y WeaponComponent).
// Envuelve a lv::core::Health. Ver docs/ARQUITECTURA.md §25.
//
// - take_damage(info, by): lo llama C++ directo (ProjectileSystem) por cada bala, perdigón, explosión o frame de
//   láser; no pasa por GDScript. Solo recibe daño si el participante es vulnerable (vivo y sin protección).
// - Señales finales para la vista y el HUD (GDScript):
//     damaged(amount, by, kind, direction) · healed(amount) · health_changed(current, max) · died(by)
//   El láser llega en trocitos cada frame: la salud baja en el acto, pero `damaged` se emite agrupado cada
//   0,25 s (lv::core::DamageBatch), así no hay una señal ni un número flotante por frame.
// - Al llegar a 0 emite died(by). Qué significa morir lo decide la partida (match.gd: pierde la vida, suelta el
//   trofeo y reaparece con LifeComponent). Al reaparecer, Participant llama reset() y vuelve a la salud máxima.
// - No dibuja: la vista (participant_view.gd) escucha las señales.
class HealthComponent : public Node {
	GDCLASS(HealthComponent, Node)

public:
	enum DamageKind {
		DAMAGE_BULLET = static_cast<int>(lv::core::DamageKind::Bullet),
		DAMAGE_EXPLOSION = static_cast<int>(lv::core::DamageKind::Explosion),
		DAMAGE_BEAM = static_cast<int>(lv::core::DamageKind::Beam),
	};

	void _ready() override;
	void _physics_process(double p_delta) override;

	// Aplica un golpe. Devuelve la salud que quitó de verdad.
	float take_damage(const lv::core::DamageInfo &p_info, Participant *p_by);
	// Versión para GDScript / pruebas (sin posición).
	float apply_damage(float p_amount, Object *p_by, int p_kind, const Vector2 &p_direction);
	float heal(float p_amount);
	// Salud llena (al reaparecer o en una partida nueva).
	void reset();

	float get_health() const { return health_.current(); }
	float get_ratio() const { return health_.ratio(); }
	bool is_dead() const { return health_.is_dead(); }
	void set_max_health(float p_value);
	float get_max_health() const { return health_.max(); }

protected:
	static void _bind_methods();

private:
	lv::core::Health health_;
	lv::core::DamageBatch beam_batch_;
	ObjectID beam_by_; // quién hizo el último trocito de láser del grupo (por id: puede desaparecer)
	Vector2 beam_direction_;

	void flush_beam();
	void emit_changed();
};

VARIANT_ENUM_CAST(HealthComponent::DamageKind);
