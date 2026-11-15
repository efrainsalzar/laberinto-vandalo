#pragma once

#include "core/maze_algorithms.h"
#include "core/random.h"

#include <vector>

namespace lv::core {

// Navegación de bots sobre las CELDAS LÓGICAS del laberinto (PassageGrid), no sobre casillas: un mapa
// Gigante tiene ~17 mil celdas frente a cientos de miles de casillas. Entre dos celdas vecinas con pasaje
// abierto, la línea recta de centro a centro siempre va por suelo, así que el camino se puede seguir tal cual.
// Ver docs/ARQUITECTURA.md §19.

// A* (algoritmo estándar de caminos más cortos) con heurística Manhattan.
// Devuelve las celdas desde p_from hasta p_to, ambas incluidas; vacío si no hay camino.
// p_extra_cost (opcional, una entrada por celda): costo adicional por ENTRAR a esa celda. Hoy no se usa;
// queda listo para terrenos (barro, agua) sin cambiar el algoritmo.
// p_max_visits acota el trabajo: si se supera devuelve vacío (presupuesto, ver docs/RENDIMIENTO.md).
std::vector<int> find_path(const PassageGrid &p_grid, int p_from, int p_to,
		const std::vector<float> *p_extra_cost = nullptr, int p_max_visits = 1 << 20);

// "Error humano" en un cruce: con probabilidad p_mistake_chance, en una celda con 3 o más salidas, toma
// otra salida abierta que no es la planeada ni por donde vino. Si no es un cruce o no se equivoca,
// devuelve p_planned_next. Así un bot fácil a veces se pierde y uno experto nunca.
int choose_next_cell(const PassageGrid &p_grid, int p_current, int p_planned_next, int p_previous,
		float p_mistake_chance, Rng &r_rng);

} // namespace lv::core
