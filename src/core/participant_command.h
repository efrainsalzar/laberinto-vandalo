#pragma once

namespace lv::core {

// Patrón Command: lo que un participante quiere hacer en este frame, como dato puro.
//
// Cualquier controlador (humano, bot, red) produce un ParticipantCommand; el participante
// solo sabe ejecutarlo. Al ser datos simples se puede enviar por red o grabar para repeticiones.
// Crecerá con nuevas acciones (usar objeto...) cuando lleguen esas fases.
struct ParticipantCommand {
	// Dirección deseada. Cada componente en [-1, 1]; (0, 0) = quieto.
	float move_x = 0.0f;
	float move_y = 0.0f;

	// Hacia dónde apunta (no hace falta que esté normalizado). (0, 0) = hacia donde mira.
	float aim_x = 0.0f;
	float aim_y = 0.0f;
	// Quiere disparar en este frame (el arma decide si puede: cadencia, munición). Ver §22.
	bool fire = false;

	bool is_idle() const { return move_x == 0.0f && move_y == 0.0f; }
	bool has_aim() const { return aim_x != 0.0f || aim_y != 0.0f; }
};

} // namespace lv::core
