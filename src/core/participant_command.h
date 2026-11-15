#pragma once

namespace lv::core {

// Patrón Command: lo que un participante quiere hacer en este frame, como dato puro.
//
// Cualquier controlador (humano, bot, red) produce un ParticipantCommand; el participante
// solo sabe ejecutarlo. Al ser datos simples se puede enviar por red o grabar para repeticiones.
// Crecerá con nuevas acciones (atacar, usar objeto...) cuando lleguen esas fases.
struct ParticipantCommand {
	// Dirección deseada. Cada componente en [-1, 1]; (0, 0) = quieto.
	float move_x = 0.0f;
	float move_y = 0.0f;

	bool is_idle() const { return move_x == 0.0f && move_y == 0.0f; }
};

} // namespace lv::core
