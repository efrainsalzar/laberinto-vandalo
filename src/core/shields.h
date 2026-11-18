#pragma once

#include "core/health.h"

#include <memory>
#include <vector>

namespace lv::core {

enum class ShieldKind {
	Bubble, // burbuja: absorbe UN golpe entero (cualquier tipo) y se rompe; contra el láser aguanta `beam_budget`
	Frontal, // frontal: anula lo que llega por delante (±half_angle de hacia donde apunta)
	Mirror, // espejo: devuelve balas y láser a quien disparó (las explosiones lo atraviesan)
};

// Qué hizo un escudo con un golpe.
enum class ShieldOutcome {
	Pass, // no lo tocó: sigue al siguiente eslabón (o a la salud)
	Absorbed, // lo anuló
	Reflected, // lo anuló y hay que devolverlo a quien lo causó
};

// Lo que un escudo necesita saber del portador al filtrar un golpe.
struct ShieldContext {
	float aim_x = 0.0f; // hacia dónde apunta (normalizada; 0,0 = sin dirección)
	float aim_y = 0.0f;
};

// Un eslabón de la cadena (patrón Chain of Responsibility): mira el golpe y decide pasarlo, anularlo o devolverlo.
// Cada escudo dura `duration` segundos (o hasta romperse). Ver docs/ARQUITECTURA.md §27.
class Shield {
public:
	explicit Shield(float p_duration) : duration_(p_duration), time_left_(p_duration) {}
	virtual ~Shield() = default;

	virtual ShieldKind kind() const = 0;
	virtual ShieldOutcome filter(const DamageInfo &p_info, const ShieldContext &p_context) = 0;
	// El escudo se gastó (la burbuja después de su golpe) o se le acabó el tiempo.
	virtual bool is_spent() const { return time_left_ <= 0.0f; }

	void tick(float p_delta) { time_left_ -= p_delta; }
	void renew() { time_left_ = duration_; }
	float time_left() const { return time_left_ > 0.0f ? time_left_ : 0.0f; }
	float duration() const { return duration_; }

private:
	float duration_ = 0.0f;
	float time_left_ = 0.0f;
};

class BubbleShield : public Shield {
public:
	// p_beam_budget: daño de láser que aguanta antes de romperse (el láser llega en trocitos cada frame;
	// sin esto la burbuja se rompería con el primer trocito).
	BubbleShield(float p_duration, float p_beam_budget = 35.0f) : Shield(p_duration), beam_budget_(p_beam_budget) {}
	ShieldKind kind() const override { return ShieldKind::Bubble; }
	ShieldOutcome filter(const DamageInfo &p_info, const ShieldContext &p_context) override;
	bool is_spent() const override { return popped_ || Shield::is_spent(); }

private:
	bool popped_ = false;
	float beam_budget_ = 35.0f;
};

class FrontalShield : public Shield {
public:
	FrontalShield(float p_duration, float p_half_angle_deg);
	ShieldKind kind() const override { return ShieldKind::Frontal; }
	ShieldOutcome filter(const DamageInfo &p_info, const ShieldContext &p_context) override;

private:
	float cos_half_ = 0.5f;
};

class MirrorShield : public Shield {
public:
	explicit MirrorShield(float p_duration) : Shield(p_duration) {}
	ShieldKind kind() const override { return ShieldKind::Mirror; }
	ShieldOutcome filter(const DamageInfo &p_info, const ShieldContext &p_context) override;
};

// Resultado de pasar un golpe por toda la cadena.
struct ShieldResult {
	ShieldOutcome outcome = ShieldOutcome::Pass;
	ShieldKind by = ShieldKind::Bubble; // qué escudo lo paró (si no es Pass)
};

// La cadena de escudos de un participante, en orden fijo: Burbuja → Frontal → Espejo (diseño aprobado).
// - add(): un escudo nuevo; si ya tenía uno del mismo tipo, solo renueva su tiempo (no se apilan).
// - filter(): pasa el golpe eslabón por eslabón; el primero que no lo deja pasar decide.
// - tick(): avanza los tiempos y quita los gastados; devuelve los tipos que se acabaron (para la vista).
// La salud no sabe nada de escudos: quien aplica el daño filtra primero y solo si pasa llama a la salud.
class ShieldChain {
public:
	void add(std::unique_ptr<Shield> p_shield);
	ShieldResult filter(const DamageInfo &p_info, const ShieldContext &p_context);
	void tick(float p_delta, std::vector<ShieldKind> &r_ended);
	// Quita los gastados (la burbuja rota) sin avanzar el tiempo. Devuelve los tipos quitados.
	void remove_spent(std::vector<ShieldKind> &r_ended);
	void clear() { links_.clear(); }

	bool has(ShieldKind p_kind) const { return find(p_kind) != nullptr; }
	const Shield *find(ShieldKind p_kind) const;
	bool empty() const { return links_.empty(); }
	const std::vector<std::unique_ptr<Shield>> &links() const { return links_; }

private:
	std::vector<std::unique_ptr<Shield>> links_; // siempre ordenados por ShieldKind
};

} // namespace lv::core
