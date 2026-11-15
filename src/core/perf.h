#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <string>

namespace lv::core {

// Registro de tiempos por sistema (instrumentación de rendimiento).
//
// Cada sistema mide su trabajo con PerfScope (RAII) bajo un nombre ("maze.plan", "maze.apply"...).
// Se acumula por "ventana" (1 s): llamadas, total, máximo. La interfaz (panel F3) y los monitores del
// depurador de Godot leen estos datos. Coste: dos lecturas de reloj por medición.
// Ver docs/RENDIMIENTO.md.
class Perf {
public:
	struct Stat {
		uint32_t calls = 0; // en la ventana actual
		double total_ms = 0.0; // en la ventana actual
		double max_ms = 0.0; // peor llamada en la ventana actual
		double last_ms = 0.0; // última llamada
		// Resultados de la ventana anterior (estables para mostrar).
		uint32_t window_calls = 0;
		double window_total_ms = 0.0;
		double window_max_ms = 0.0;
	};

	static Perf &instance();

	void record(const std::string &p_name, double p_ms);
	// Tiempo acumulado por sistema desde la última llamada (≈ el último frame) y reinicia el acumulador.
	// Permite saber QUÉ sistema causó un tirón en el frame exacto en que ocurrió.
	std::map<std::string, double> take_frame();
	// Cierra la ventana actual (llamar una vez por segundo): los valores pasan a window_*.
	void roll_window();
	void clear();
	const std::map<std::string, Stat> &stats() const { return stats_; }

private:
	std::map<std::string, Stat> stats_;
	std::map<std::string, double> frame_;
};

// Mide el tiempo de un bloque: { PerfScope s("maze.plan"); ... }.
class PerfScope {
public:
	explicit PerfScope(const char *p_name) :
			name_(p_name), start_(std::chrono::steady_clock::now()) {}
	~PerfScope() {
		const auto end = std::chrono::steady_clock::now();
		Perf::instance().record(name_, std::chrono::duration<double, std::milli>(end - start_).count());
	}
	PerfScope(const PerfScope &) = delete;
	PerfScope &operator=(const PerfScope &) = delete;

private:
	const char *name_;
	std::chrono::steady_clock::time_point start_;
};

} // namespace lv::core
