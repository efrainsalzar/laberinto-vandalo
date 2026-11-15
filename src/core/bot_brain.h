#pragma once

#include "core/cell.h"
#include "core/random.h"

#include <memory>
#include <vector>

namespace lv::core {

// Cerebro de un bot con Utility AI (IA por puntajes, la técnica de The Sims): en cada decisión puntúa
// todas sus acciones posibles con lo que sabe del mundo y elige la mejor.
//
// - Cada acción es una clase pequeña (Strategy): BotAction. Una mecánica nueva (disparar, usar portal,
//   recoger un power-up nuevo) = una acción nueva que se registra con add_action(); el resto no cambia.
// - La dificultad son datos (BotTuning): tiempo de reacción, cada cuánto piensa, errores, interés por los
//   objetos y variación ("a veces pro, a veces noob", VISION §9).
// - No sabe nada de Godot ni de caminos: decide QUÉ hacer y DÓNDE (una casilla); el controlador navega.
// Ver docs/ARQUITECTURA.md §19.

// Perillas de dificultad (vienen de BotProfile .tres).
struct BotTuning {
	float reaction_time = 0.5f; // segundos entre "decidir algo distinto" y hacerlo
	float think_interval = 0.6f; // cada cuánto vuelve a evaluar sus acciones
	float mistake_chance = 0.10f; // probabilidad de equivocarse en un cruce (ver choose_next_cell)
	float pickup_interest = 0.5f; // 0..1: cuánto lo distraen los objetos
	float variation = 0.10f; // ruido en los puntajes (impredecible)
};

// Lo que el bot sabe del mundo en el momento de pensar (lo arma el controlador, en casillas).
struct BotWorld {
	Cell self;
	TileRect base; // su zona de resguardo
	bool carrying_trophy = false; // lo lleva él
	bool trophy_available = false; // está en la bóveda o en el piso (se puede tomar)
	bool trophy_carried_by_other = false; // lo lleva otro participante (se le puede quitar)
	Cell trophy; // dónde está (en la bóveda, en el piso o sobre su portador)
	std::vector<Cell> seen_pickups; // objetos que VE ahora (juego limpio: solo con línea de visión)
	Cell explore_target; // a dónde ir si no hay nada mejor (lo elige el controlador al azar)
	float time_left = -1.0f; // segundos de partida (-1 = sin límite); para acciones futuras
};

enum class BotGoal {
	None,
	GetTrophy,
	DeliverTrophy,
	CollectPickup,
	Explore,
	StealTrophy,
};

struct BotDecision {
	BotGoal goal = BotGoal::None;
	Cell target;
	float score = 0.0f;
	bool operator==(const BotDecision &p_o) const { return goal == p_o.goal && target == p_o.target; }
};

// Una acción posible. score() en 0..1 aprox. (0 = no tiene sentido ahora); target() = casilla destino.
class BotAction {
public:
	virtual ~BotAction() = default;
	virtual BotGoal goal() const = 0;
	virtual float score(const BotWorld &p_world, const BotTuning &p_tuning) const = 0;
	virtual Cell target(const BotWorld &p_world) const = 0;
};

class GetTrophyAction : public BotAction {
public:
	BotGoal goal() const override { return BotGoal::GetTrophy; }
	float score(const BotWorld &p_world, const BotTuning &p_tuning) const override;
	Cell target(const BotWorld &p_world) const override { return p_world.trophy; }
};

// Persigue al portador para quitarle el trofeo (robo por contacto). El destino se mueve: el bot replantea
// el camino cada vez que piensa.
class StealTrophyAction : public BotAction {
public:
	BotGoal goal() const override { return BotGoal::StealTrophy; }
	float score(const BotWorld &p_world, const BotTuning &p_tuning) const override;
	Cell target(const BotWorld &p_world) const override { return p_world.trophy; }
};

class DeliverTrophyAction : public BotAction {
public:
	BotGoal goal() const override { return BotGoal::DeliverTrophy; }
	float score(const BotWorld &p_world, const BotTuning &p_tuning) const override;
	Cell target(const BotWorld &p_world) const override { return p_world.base.center(); }
};

// Va por el objeto visto más cercano. Su atractivo baja con la distancia y sube con pickup_interest.
class CollectPickupAction : public BotAction {
public:
	static constexpr float RANGE_TILES = 12.0f; // más lejos que esto no le interesa
	BotGoal goal() const override { return BotGoal::CollectPickup; }
	float score(const BotWorld &p_world, const BotTuning &p_tuning) const override;
	Cell target(const BotWorld &p_world) const override;
};

class ExploreAction : public BotAction {
public:
	static constexpr float BASE_SCORE = 0.1f;
	BotGoal goal() const override { return BotGoal::Explore; }
	float score(const BotWorld &, const BotTuning &) const override { return BASE_SCORE; }
	Cell target(const BotWorld &p_world) const override { return p_world.explore_target; }
};

class BotBrain {
public:
	static constexpr float KEEP_BONUS = 0.05f; // inercia: no cambia de idea por diferencias mínimas

	explicit BotBrain(const BotTuning &p_tuning);
	// Crea un cerebro con las acciones de hoy (trofeo, entregar, quitarlo, objetos, explorar).
	static std::unique_ptr<BotBrain> make_default(const BotTuning &p_tuning);

	void add_action(std::unique_ptr<BotAction> p_action);
	const BotTuning &tuning() const { return tuning_; }

	// Puntúa todas las acciones y devuelve la mejor (con la variación de la dificultad).
	BotDecision evaluate(const BotWorld &p_world, Rng &r_rng) const;

	// Llamar cada frame. Piensa cada think_interval; un objetivo NUEVO se adopta tras reaction_time
	// (el primero, de inmediato). Si solo se movió el destino del mismo objetivo, se actualiza al pensar.
	// Devuelve true cuando cambia la decisión vigente (el controlador debe recalcular el camino).
	bool update(float p_delta, const BotWorld &p_world, Rng &r_rng);
	// ¿Pensará en el próximo update(p_delta)? Sirve para armar BotWorld solo cuando hace falta.
	bool will_think(float p_delta) const { return think_timer_ - p_delta <= 0.0f; }
	// Pensar en el próximo update (p. ej. llegó al destino o el mapa cambió).
	void think_soon() { think_timer_ = 0.0f; }
	// Desfase inicial para que no piensen todos los bots en el mismo frame.
	void set_think_offset(float p_seconds) { think_timer_ = p_seconds; }

	const BotDecision &current() const { return current_; }
	bool has_pending() const { return has_pending_; }

private:
	BotTuning tuning_;
	std::vector<std::unique_ptr<BotAction>> actions_;
	BotDecision current_;
	BotDecision pending_;
	bool has_pending_ = false;
	float pending_timer_ = 0.0f;
	float think_timer_ = 0.0f;
};

} // namespace lv::core
