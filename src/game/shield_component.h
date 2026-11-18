#pragma once

#include "core/shields.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace godot;

// Escudos de un participante, como componente hijo (composición, junto a HealthComponent). Envuelve a
// lv::core::ShieldChain (Chain of Responsibility: Burbuja → Frontal → Espejo). Ver docs/ARQUITECTURA.md §27.
//
// - add_shield(kind, duration): lo llama un ShieldEffect al recoger un objeto (o la tecla de prueba 9).
//   duration <= 0 = la duración por defecto de ese escudo (propiedades del componente).
// - filter(info): lo llama C++ (ProjectileSystem) ANTES de la salud. Si un escudo lo anula o lo devuelve, la
//   salud ni se entera. Quien llama se encarga de devolver el disparo (es quien sabe de balas).
// - Señales para la vista y la tarjeta: shield_added(kind) · shield_ended(kind) · blocked(kind, reflected).
//   `blocked` del láser se emite como mucho cada 0,25 s (el láser pega cada frame).
// - Al perder la vida se pierden los escudos.
class ShieldComponent : public Node {
	GDCLASS(ShieldComponent, Node)

public:
	enum Kind {
		SHIELD_BUBBLE = static_cast<int>(lv::core::ShieldKind::Bubble),
		SHIELD_FRONTAL = static_cast<int>(lv::core::ShieldKind::Frontal),
		SHIELD_MIRROR = static_cast<int>(lv::core::ShieldKind::Mirror),
	};

	void _ready() override;
	void _physics_process(double p_delta) override;

	void add_shield(int p_kind, float p_duration);
	lv::core::ShieldResult filter(const lv::core::DamageInfo &p_info);
	void clear();

	bool has_shield(int p_kind) const;
	float get_time_left(int p_kind) const;
	// [{kind, remaining, duration}] (para la tarjeta y la vista).
	Array get_shields() const;
	bool is_empty() const { return chain_.empty(); }

	void set_bubble_duration(float p_value) { bubble_duration_ = p_value; }
	float get_bubble_duration() const { return bubble_duration_; }
	void set_frontal_duration(float p_value) { frontal_duration_ = p_value; }
	float get_frontal_duration() const { return frontal_duration_; }
	void set_frontal_half_angle(float p_value) { frontal_half_angle_ = p_value; }
	float get_frontal_half_angle() const { return frontal_half_angle_; }
	void set_mirror_duration(float p_value) { mirror_duration_ = p_value; }
	float get_mirror_duration() const { return mirror_duration_; }

protected:
	static void _bind_methods();

private:
	static constexpr float BEAM_FEEDBACK_SECONDS = 0.25f;

	lv::core::ShieldChain chain_;
	std::vector<lv::core::ShieldKind> ended_; // reutilizado
	float beam_feedback_cooldown_ = 0.0f;
	float bubble_duration_ = 20.0f;
	float frontal_duration_ = 8.0f;
	float frontal_half_angle_ = 60.0f;
	float mirror_duration_ = 5.0f;

	void emit_ended();
};

VARIANT_ENUM_CAST(ShieldComponent::Kind);
