#include "register_types.h"

#include "version.h"

#include "game/bot_controller.h"
#include "game/bot_coordinator.h"
#include "game/entity_index.h"
#include "game/match_controller.h"
#include "game/maze.h"
#include "game/maze_profile.h"
#include "game/maze_shifter.h"
#include "game/participant.h"
#include "game/participant_controller.h"
#include "game/perf_stats.h"
#include "game/pickup.h"
#include "game/pickup_factory.h"
#include "game/pickup_spawn_policy.h"
#include "game/pickup_spawner.h"
#include "game/pickup_type.h"
#include "game/stats_component.h"
#include "game/stats_profile.h"
#include "game/trophy_controller.h"
#include "game/vision.h"

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
	// Rendimiento
	GDREGISTER_CLASS(PerfStats);
	GDREGISTER_CLASS(EntityIndex);

	// Laberinto
	GDREGISTER_CLASS(MazeProfile);
	GDREGISTER_CLASS(Maze);
	GDREGISTER_CLASS(MazeShifter);

	// Stats
	GDREGISTER_CLASS(StatsProfile);
	GDREGISTER_CLASS(StatsComponent);

	// Participantes (la clase base va antes que sus derivadas)
	GDREGISTER_ABSTRACT_CLASS(ParticipantController);
	GDREGISTER_CLASS(HumanController);
	GDREGISTER_CLASS(WanderController);
	GDREGISTER_CLASS(BotController);
	GDREGISTER_CLASS(Participant);

	// Visión
	GDREGISTER_CLASS(Vision);

	// Objetos recolectables
	GDREGISTER_CLASS(PickupEffect);
	GDREGISTER_CLASS(PickupType);
	GDREGISTER_CLASS(Pickup);
	GDREGISTER_CLASS(PickupFactory);
	GDREGISTER_ABSTRACT_CLASS(PickupSpawnPolicy);
	GDREGISTER_CLASS(KeepDensityPolicy);
	GDREGISTER_CLASS(PickupSpawner);

	// Trofeo y bases
	GDREGISTER_CLASS(TrophyController);

	// Bots
	GDREGISTER_CLASS(BotProfile);
	GDREGISTER_CLASS(BotCoordinator);

	// Partida: reloj y fases
	GDREGISTER_CLASS(MatchRules);
	GDREGISTER_CLASS(MatchController);

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
