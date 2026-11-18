# Hoja de ruta y estado

La hoja de ruta completa del juego está en [VISION.md §17](VISION.md#17-hoja-de-ruta).
Aquí se lleva el **estado real** y el **siguiente paso**.

## Estado actual

**v0.0.31** · 138 tests · Etapa 1 completa y Etapa 3 (bots) adelantada. Últimas piezas: lenguaje visual de objetos
(v0.0.22), armas en el mapa + HUD (v0.0.26), tarjeta del participante (v0.0.29), arreglo de la muerte (v0.0.30) y HUD
flotante (v0.0.31). **Siguiente: Etapa 4 — Combate, Pieza 1 (salud y daño). Diseño ya aprobado: ver abajo.** Todo lo terminado, versión por versión: [HISTORIAL.md](HISTORIAL.md).

## Plan acordado (etapas, una pieza a la vez)

Bloques de interfaz ya hechos: ~~objetos infinitos~~ ✅ · ~~HUD limpio + textos flotantes~~ ✅.
Colores, arte e identidad visual se dejan para la etapa 7 (decisión del dueño del proyecto).
Ideas del mapa en detalle: [`DISENO_MAPA.md`](DISENO_MAPA.md).

### Etapa 1 — Jugable de principio a fin
1. ~~Bases + trofeo + ganar~~ ✅
2. ~~Partida: reloj, fases, victoria y fin por tiempo~~ ✅ (v0.0.16)
3. ~~Pantallas: inicio, configurar, pausa, resultados, opciones~~ ✅

~~Mapas que cambian en tiempo real~~ ✅ (v0.0.12, en prueba en vivo).

**Etapa 1 completa. Etapa 3 (bots) adelantada ✅.** Orden acordado (dueño del proyecto, 2026-11-18):
1. ~~Economía de objetos~~ ✅ (v0.0.21)
2. ~~Lenguaje visual de objetos~~ ✅ (v0.0.22)
3. ~~Disparo + pistola inicial~~ ✅ (v0.0.23)
4. ~~Armas temporales del mapa~~ ✅ (v0.0.24–v0.0.26: disparo más real, catálogo, armas en el mapa y HUD)
5. **Combate (Etapa 4)** ← siguiente (diseño aprobado el 2026-11-18, ver "Etapa 4" abajo) → objetos que cambian el
   mapa, paredes rompibles y portales (Etapa 2.2–2.4) → formas de mapa (Etapa 2.1).

Prioridad del dueño (2026-11-18): **lo que siente el jugador real**. Los bots se quedan con lo básico (ir por el
trofeo y volver a su base); "bots que buscan armas a propósito" y los choques entre participantes quedan para después
(ver "Ideas anotadas").

Cada mecánica entra como **acción del bot** + **dato .tres**.

### Etapa 2 — El mundo
1. `MapConfig` + formas de contorno (cuadrado, círculo, hexágono...). (Algoritmos, dificultad y tamaños ✅ en v0.0.10.)
2. Áreas conectadas por puertas → **la bóveda como mini-mapa propio**; las puertas sirven para portales.
3. Terrenos como datos: agua, barro, hielo (inercia), ríos con puentes, árboles/rocas.
4. Mapa que cambia: paredes rompibles, atajos, pasadizos secretos, paredes móviles.
5. Geometría avanzada: laberintos circulares de anillos, hexagonales, paredes diagonales/curvas.

### Etapa 3 — Bots ✅ (base en v0.0.17)
Hecho: `BotController`, A* con costos, Utility AI, dificultades. Falta: personalidades (pesos por acción en .tres),
árboles de comportamiento para vándalos/guardianes, bots que usen armas/portales (una `BotAction` por mecánica).

### Etapa 4 — Combate (diseño aprobado por el dueño, 2026-11-18)
Una pieza a la vez, con el flujo de `/nuevo-modulo` (el diseño ya está aprobado: se puede pasar directo a core + tests
de cada pieza; si algo cambia respecto de lo de aquí, mostrarlo antes).

**Idea central — paquete de daño que pasa por filtros:**
```
Arma ─► DamageInfo ─► [Burbuja] ─► [Frontal] ─► [Espejo] ─► Salud ─► ¿0? ─► pierde la vida / reaparece
```
| Patrón | Dónde | Por qué |
|---|---|---|
| **Value Object** (como Command) | `DamageInfo` (core): cantidad, quién, dirección, tipo (bala / explosión / rayo), punto | Dato simple que viaja; sirve para red y repeticiones. |
| **Chain of Responsibility** (cadena de modificadores) | Escudos (pieza 3) | Cada escudo reduce, anula o devuelve el daño. Escudo nuevo = eslabón nuevo; la salud no cambia. |
| **Composición** | `HealthComponent` en el participante (junto a `LifeComponent`, `WeaponComponent`) | Igual que los demás componentes. |
| **Strategy + datos** | Efectos de objetos (pieza 2): curar, dar escudo, dar arma, dar bomba; cada uno `.tres` con `apply()` | Hoy `PickupEffect` solo cambia stats; los `.tres` actuales se migran. |
| **Datos en bloque + pool** (como `ProjectileWorld`) | Minas y bombas (pieza 4) | Un sistema para todas, con tope; reutiliza `explosion_victims`. |
| **Object Pool** (existe) | Números de daño (`FloatingTextLayer`) | Sin crear nodos por golpe. |

**Rendimiento:** el daño se calcula en C++ (`core`, con tests). Balas, explosiones y láser llaman directo al
`HealthComponent` desde `ProjectileSystem` (sin pasar por GDScript por perdigón ni por frame de láser). GDScript solo
recibe señales finales (`damaged`, `healed`, `died`) para la vista y el HUD. Mediciones nuevas con `PerfScope`.

**Decisiones del dueño:**
- **Salud 100.** **Vidas infinitas por ahora** (no hay "fuera de la partida"; las vidas limitadas quedan para después).
- Al llegar a 0: se pierde la vida → reaparece en su base con 100 y protección (ya existe `LifeComponent`); el trofeo
  se suelta al **perder la vida**, no con cada golpe (`TrophyController.eliminate_participant`).
- Hoy `ProjectileSystem.participant_hit` → `match.gd` elimina directo: la pieza 1 cambia eso por daño.
- **Pociones: se usan al recogerlas** y recuperan salud.
- **Daño como dato** en cada `WeaponType` (.tres), valores iniciales (se ajustan después sin código):

| Arma | Daño |
|---|---|
| Pistola | 20 por bala (5 balas eliminan) |
| Escopeta | 12 por perdigón × 7 |
| Láser | 70 por segundo de contacto (daño × delta por frame) |
| Bazuca | 80 en el centro → 30 en el borde de la explosión |

**Piezas (en orden):**
1. **Salud y daño por arma.** `core`: `Health` + `DamageInfo` (+ caída del daño de la explosión). `game`:
   `HealthComponent`; `WeaponSpec.damage` (y `damage_edge` para explosiones); `ProjectileSystem` aplica daño;
   al morir → `LifeComponent` (pierde la vida, reaparece con 100). Bots: sin cambios de conducta.
   Visual: destello blanco del cuerpo al recibir golpe (0,08 s), números de daño flotantes (los del láser agrupados
   cada 0,25 s), barra de salud chica sobre los rivales solo unos segundos tras dañarlos, borde rojo de pantalla con
   salud < 30, sacudida leve de cámara con explosiones cercanas, y **sección de salud en la tarjeta** del participante
   (Composite: una sección nueva, prevista en §24; sin corazones mientras las vidas sean infinitas).
2. **Pociones** (y efectos de objeto como Strategy): pequeña +25 (común), grande +60 (rara); destello verde y "+25".
3. **Escudos** (cadena de modificadores): burbuja (absorbe 1 golpe), frontal (±60° hacia donde apuntas, X s), espejo
   (devuelve el disparo, X s). Visual: anillo / arco / anillo plateado; fichas con tiempo en la tarjeta. El escudo
   anti-robo es regla del trofeo → etapa de vándalos.
4. **Bombas y minas** (objeto usable): `ParticipantCommand.use_item` (clic derecho / Q / botón del mando), ranura de
   objeto en la tarjeta con cantidad. Bomba: explota a los 2 s (mecha que parpadea). Mina: se arma en 1 s, explota
   cuando pasa un rival cerca; la ve su dueño, los rivales solo de muy cerca. Topes: 3 por jugador, 32 en el mapa
   (la más vieja desaparece). Explosión = la de la bazuca (radio, paredes protegen, daño que baja al borde).

### Etapa 5 — Seguridad de la bóveda
Soldados guardianes (mini jefes).

### Etapa 6 — Vándalos y Director de IA
Robo, burlas, saqueo, emboscadas coordinadas; el Director controla apariciones, eventos y objetos.

### Etapa 7 — Presentación
Identidad visual (paleta, fuente, Theme, glow, Forward+), avatares, arte, sonido, partículas, cámara (Phantom Camera).

### Después
Demo en itch.io · aliados · torneo con rondas · multijugador local · en línea · Steam (VISION §17).
## Ideas anotadas (no implementar todavía)

- **Bots que buscan armas a propósito** (`BotAction` + distancia preferida por arma): pospuesto por el dueño (2026-11-18).
- **Vidas limitadas** (3 vidas, con 0 fuera de la partida / saqueo de los vándalos): por ahora vidas infinitas.
- **Prueba de estrés del bloque de armas** (muchas balas, explosiones y rayos): pendiente antes de cerrar la Etapa 4.

- **Objetos de mapa con efectos especiales:** hacer temblar todo el mapa, romper paredes con formas geométricas
  (cruz, anillo, línea). Encajan en la Etapa 2.4: cada "forma de romper" es un dato que encola cambios en `Maze.queue_passage_change`.
- **Actores que buscan armas o te las roban** (vándalos, Etapa 6): una `BotAction` por conducta.
- **Historia dinámica:** el Director de IA tendrá una política propia por categoría de objetos (más armas en el Golpe, etc.).

- **Política del Director para objetos:** el Director de IA (fase 5) tendrá su propia `PickupSpawnPolicy` (p. ej. más objetos cerca de quien va perdiendo).
- **Más tipos de objeto** (pociones, escudos, bombas): solo nuevos .tres + los efectos que falten en la fase de combate.

- **Multijugador local y visión:** solo hay una luz/oscuridad por pantalla; con varios humanos en la misma máquina hará falta pantalla dividida u otra solución.
- **Partes de cada fase en el menú:** hoy se ajustan en `data/match/default_rules.tres`; se podría exponer en "Personalizado".
- **Bóveda proporcional al pasillo:** el radio de la bóveda está en casillas; con pasillos de 5, una bóveda de radio 2 mide lo mismo que un pasillo. Podría medirse en celdas.
- **Qué pasa en cada fase:** vándalos y guardianes por fase (Etapas 5–6), bases que se mueven en el Golpe.
- **Ideas del dueño del proyecto (en orden, una pieza a la vez):** botones de acción del participante (disparar,
  usar objeto) como nuevos campos de `ParticipantCommand`; power-ups nuevos (.tres); romper paredes; portales
  (puertas de la Etapa 2.2); objetos que al comerlos cambian el mapa y los caminos (usan `Maze.queue_passage_change`).
  Cada mecánica trae su `BotAction` para que los bots también la usen.
- **Especificación propia de cada arma** (dueño): láser que se mantiene / se carga, daño propio de la bazuca, rebotes, etc. Cada una como opción nueva de `WeaponSpec`.
- **Explosiones que rompen paredes** (Etapa 2.4): `ProjectileSystem.exploded` + `Maze.queue_passage_change`.
- **Impacto de bala:** hoy elimina (para probar). En la Etapa 4 quitará salud: solo cambia quien escucha `participant_hit`.
- **Balas con índice espacial:** con los vándalos (decenas de blancos) `ProjectileSystem` consultará `EntityIndex` por bala en vez de revisar todos.
- **Feedback del disparo:** destello, retroceso, chispa al chocar con pared y sonido (Etapa 7; `WeaponComponent.fired` ya existe).
- **Bots que esquivan balas** o se cubren detrás de paredes: una `BotAction` más cuando haga falta.
- **Choques entre participantes:** con el combate (Etapa 4). Hoy se atraviesan. El robo básico por contacto ya existe (v0.0.18).
- **Bots que huyen con el trofeo** (esquivar a quien los persigue / evitar emboscadas): una `BotAction` más cuando haga falta.
- **Forcejeo** (mantener contacto para robar): con el combate, como otra forma de robo en `TrophyRules`.
