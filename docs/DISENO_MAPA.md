# Diseño del mapa (mundo)

> Ideas del dueño del proyecto sobre el mapa y cómo se implementarían. Cada idea indica su **etapa** en el
> [ROADMAP](ROADMAP.md). Nada de esto se implementa fuera de su turno; cuando llegue, se diseña en detalle
> con la skill `/nuevo-modulo` y se documenta en [ARQUITECTURA.md](ARQUITECTURA.md).

## Visión
El torneo ocurre **dentro de un mundo vivo y configurable**: los participantes salen de los extremos, van al
centro (la bóveda) por el trofeo y lo llevan a su base. El mapa no tiene por qué ser cuadrado ni estático.

## Principio de arquitectura
**Todo cambio del mapa pasa por un solo lugar** (el modelo del mapa en `core`) y se anuncia con un evento
"el mapa cambió". Colisión, sombras, visión, objetos y caminos de los bots escuchan ese evento y se actualizan
(Observer). Así romper una pared, mover un muro o abrir un pasadizo no rompe ningún otro sistema.

---

## 1. Configuración del mapa — Etapa 2.1
- Un recurso `MapConfig` (`.tres`): tamaño, ancho de pasillo, bucles, tamaño de bóveda, cantidad de objetos,
  forma, terrenos habilitados. Varios mapas = varios archivos.
- **Mapa más grande** que el actual (20×14 celdas): por ejemplo 30×20 o más, según rendimiento medido.

## 2. Formas del mapa
| Nivel | Qué | Cómo | Etapa |
|---|---|---|---|
| **A. Contorno** | Cuadrado, rectángulo, círculo, hexágono, cruz, formas orgánicas | Máscara sobre la cuadrícula: las celdas fuera de la forma no existen. El algoritmo del laberinto solo recorre celdas válidas. Funciona con todo lo actual. | 2.1 |
| **B. Geometría real** | Laberinto circular de **anillos concéntricos**, **hexagonal**, paredes **diagonales o curvas** | Separar **topología** (grafo de celdas y conexiones: el algoritmo no cambia) de **geometría** (paredes como polígonos con colisión y oclusores; visión con rayos contra segmentos en lugar de casillas). Búsqueda de caminos sobre el grafo. | 2.5 |

## 3. Áreas conectadas y la bóveda interior — Etapa 2.2
- La **bóveda es su propio mini-mapa** (con estructura propia y soldados guardianes en la etapa 5).
- Como varios participantes pueden estar dentro y fuera a la vez, **no es un cambio de pantalla**: todas las
  áreas existen al mismo tiempo en el mundo, separadas, y se conectan con **puertas** (pares de entrada/salida).
  La cámara sigue al jugador de un área a otra con una transición.
- El mismo sistema de puertas sirve para **portales** y **pasadizos secretos**.
- Mientras tanto (etapa 1), la bóveda es la sala abierta del centro. El trofeo se diseña sin depender de dónde
  está la bóveda (recibe una posición), para mudarlo al mini-mapa sin rehacerlo.

## 4. Terrenos — Etapa 2.3
Cada terreno es un **dato** (`TerrainType.tres`): efecto sobre el movimiento, si bloquea el paso, si tapa la
visión, costo para la búsqueda de caminos de los bots.

| Terreno | Efecto |
|---|---|
| Agua poco profunda / barro | Más lento mientras se está encima (modificador de Stats) |
| Hielo / suelo resbaladizo | Inercia: cuesta frenar y girar (el movimiento pasa a tener aceleración y fricción por terreno) |
| Río / agua profunda | Infranqueable salvo por **puentes**: rutas obligadas y emboscadas |
| Árboles, rocas, cajas | Bloquean el paso y la visión |

## 5. Mapa que cambia durante la partida — Etapa 2.4
| Elemento | Cómo |
|---|---|
| **Paredes rompibles** | Pared con resistencia; al romperse pasa a suelo → abre un **atajo**. Evento "mapa cambió". |
| **Pasadizos secretos** | Parecen pared pero se pueden atravesar (o se abren con un disparador). |
| **Atajos de un solo sentido** | Puertas que solo se cruzan en una dirección. |
| **Paredes que se mueven** | El "laberinto vivo": el Director (etapa 6) abre y cierra caminos con aviso previo. |
| **Bases que se mueven** | Fase 3 de la partida (VISION §4). |

## 6. Rendimiento
- Lógica del mapa en C++ y en cuadrícula/grafo (consultas baratas), no en física.
- Al cambiar el mapa se actualiza solo la zona afectada, no todo el mapa.
- Medir con el profiler antes de optimizar; el tamaño máximo del mapa se decide midiendo.
