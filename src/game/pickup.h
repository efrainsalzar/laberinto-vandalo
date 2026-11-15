#pragma once

#include "game/pickup_type.h"

#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/node2d.hpp>

using namespace godot;

class Participant;

// Un objeto recolectable en el laberinto. Lo crea PickupFactory a partir de un PickupType.
//
// Cuando un Participant lo toca: aplica los efectos de su tipo como modificadores de Stats,
// emite `collected` (Observer) y desaparece. No sabe quién escucha ni dibuja nada: la vista
// vive en su escena (pickup.tscn). Ver docs/ARQUITECTURA.md §8.
class Pickup : public Area2D {
	GDCLASS(Pickup, Area2D)

public:
	void _ready() override;

	void set_type(const Ref<PickupType> &p_type) { type_ = p_type; }
	Ref<PickupType> get_type() const { return type_; }

	// Aplica los efectos a un participante y se consume. Público para poder usarlo
	// sin física (tests, IA, red). Devuelve false si ya estaba consumido.
	bool collect(Participant *p_participant);

protected:
	static void _bind_methods();

private:
	Ref<PickupType> type_;
	bool consumed_ = false;

	void on_body_entered(Node2D *p_body);
};
