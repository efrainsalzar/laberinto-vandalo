#pragma once

#include "core/participant_command.h"
#include "core/random.h"

#include <memory>

#include <godot_cpp/classes/ref_counted.hpp>

using namespace godot;

class Participant;

// Patrón Strategy: decide qué hace un participante en cada frame.
//
// Participant tiene un controlador y le pide una orden (ParticipantCommand) en cada frame,
// sin saber si lo controla una persona, un bot o la red. Para un nuevo tipo de control
// se hereda de esta clase; Participant no cambia. Ver docs/ARQUITECTURA.md §5.
class ParticipantController : public RefCounted {
	GDCLASS(ParticipantController, RefCounted)

public:
	// Devuelve la orden para este frame. La implementación base no hace nada (quieto).
	virtual lv::core::ParticipantCommand compute_command(const Participant &p_participant, double p_delta);
	// El participante volvió a su base tras ser eliminado (p. ej. un bot debe replantear su camino).
	virtual void on_respawned(const Participant &p_participant) {}

protected:
	static void _bind_methods() {}
};

// Controlador de una persona con teclado (o mando): lee las acciones move_* del InputMap.
class HumanController : public ParticipantController {
	GDCLASS(HumanController, ParticipantController)

public:
	lv::core::ParticipantCommand compute_command(const Participant &p_participant, double p_delta) override;

protected:
	static void _bind_methods() {}
};

// Deambula al azar: cambia de dirección cada 0,4-1,6 s o al quedar trabado contra una pared.
// Otra estrategia (Strategy) más: se usa en la prueba de estrés y anticipa a los bots (etapa 3).
class WanderController : public ParticipantController {
	GDCLASS(WanderController, ParticipantController)

public:
	lv::core::ParticipantCommand compute_command(const Participant &p_participant, double p_delta) override;

protected:
	static void _bind_methods() {}

private:
	std::unique_ptr<lv::core::Rng> rng_;
	float timer_ = 0.0f;
	float dir_x_ = 0.0f;
	float dir_y_ = 0.0f;

	void pick_direction();
};