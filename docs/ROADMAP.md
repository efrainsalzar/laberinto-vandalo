# Hoja de ruta y estado

La hoja de ruta completa del juego está en [VISION.md §17](VISION.md#17-hoja-de-ruta).
Aquí se lleva el **estado real** y el **siguiente paso**.

## Estado actual

**v0.0.23** · 128 tests · Etapa 1 completa y Etapa 3 (bots) adelantada. Últimas piezas: lenguaje visual de objetos
(v0.0.22) y disparo + pistola inicial (v0.0.23). Todo lo terminado, versión por versión: [HISTORIAL.md](HISTORIAL.md).

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
4. **Armas temporales del mapa** ← siguiente (categoría "armas"; ficha con tiempo restante en el HUD; bots que las buscan).
5. Combate (Etapa 4) → objetos que cambian el mapa, paredes rompibles y portales (Etapa 2.2–2.4) → formas de mapa (Etapa 2.1).

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

### Etapa 4 — Combate
Salud, 3 vidas (sobre `LifeCycle`, ya hay reaparición y soltar el trofeo al ser eliminado — v0.0.20); armas, bombas, minas, escudos, pociones como datos.

### Etapa 5 — Seguridad de la bóveda
Soldados guardianes (mini jefes).

### Etapa 6 — Vándalos y Director de IA
Robo, burlas, saqueo, emboscadas coordinadas; el Director controla apariciones, eventos y objetos.

### Etapa 7 — Presentación
Identidad visual (paleta, fuente, Theme, glow, Forward+), avatares, arte, sonido, partículas, cámara (Phantom Camera).

### Después
Demo en itch.io · aliados · torneo con rondas · multijugador local · en línea · Steam (VISION §17).
## Ideas anotadas (no implementar todavía)

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
- **Impacto de bala:** hoy elimina (para probar). En la Etapa 4 quitará salud: solo cambia quien escucha `participant_hit`.
- **Balas con índice espacial:** con los vándalos (decenas de blancos) `ProjectileSystem` consultará `EntityIndex` por bala en vez de revisar todos.
- **Feedback del disparo:** destello, retroceso, chispa al chocar con pared y sonido (Etapa 7; `WeaponComponent.fired` ya existe).
- **Bots que esquivan balas** o se cubren detrás de paredes: una `BotAction` más cuando haga falta.
- **Choques entre participantes:** con el combate (Etapa 4). Hoy se atraviesan. El robo básico por contacto ya existe (v0.0.18).
- **Bots que huyen con el trofeo** (esquivar a quien los persigue / evitar emboscadas): una `BotAction` más cuando haga falta.
- **Forcejeo** (mantener contacto para robar): con el combate, como otra forma de robo en `TrophyRules`.
