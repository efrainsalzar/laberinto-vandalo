#include "register_types.h"

#include "version.h"

#include "game/maze.h"
#include "game/participant.h"
#include "game/participant_controller.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// Punto único donde se registran todas las clases C++ visibles para Godot.
// Cada módulo nuevo agrega aquí su GDREGISTER_CLASS(...).
void initialize_laberinto_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	// Laberinto
	GDREGISTER_CLASS(Maze);

	// Participantes (la clase base va antes que sus derivadas)
	GDREGISTER_ABSTRACT_CLASS(ParticipantController);
	GDREGISTER_CLASS(HumanController);
	GDREGISTER_CLASS(Participant);

	UtilityFunctions::print("[laberinto] Extension C++ cargada (v", LV_VERSION, ")");
}

void uninitialize_laberinto_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

extern "C" {
// Punto de entrada; el nombre debe coincidir con `entry_symbol` en project/bin/laberinto.gdextension.
GDExtensionBool GDE_EXPORT laberinto_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
	init_obj.register_initializer(initialize_laberinto_module);
	init_obj.register_terminator(uninitialize_laberinto_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
