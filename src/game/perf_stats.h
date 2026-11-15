#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

// Acceso desde GDScript a las mediciones de lv::core::Perf (métodos estáticos).
//   PerfStats.snapshot() -> { "maze.plan": {calls, total_ms, max_ms, last_ms}, ... }  (ventana anterior, 1 s)
//   PerfStats.roll_window()  -> cierra la ventana (lo llama el panel de rendimiento una vez por segundo)
// Ver docs/RENDIMIENTO.md.
class PerfStats : public Object {
	GDCLASS(PerfStats, Object)

public:
	static Dictionary snapshot();
	static void roll_window();
	static void clear();

protected:
	static void _bind_methods();
};
