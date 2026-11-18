# Historial de versiones

> Lo ya terminado, de lo más nuevo a lo más viejo. El estado actual y lo que sigue están en
> [ROADMAP.md](ROADMAP.md). Al cerrar una pieza se agrega su bloque **arriba**.

### v0.0.33 — Tarjeta del boceto arriba a la izquierda ✅
- [x] Vuelve el diseño del boceto (avatar grande, píldoras, dos ranuras de arma BASE / ESPECIAL, efectos dentro), ahora arriba a la izquierda.
- [x] Sección SAL con barra redondeada; el anillo del avatar muestra la salud. Sigue volviéndose transparente si hay alguien detrás.

### v0.0.32 — Etapa 4.1: salud y daño por arma ✅
- [x] Salud 100 (`HealthComponent` + `lv::core::Health`). Una bala ya no elimina: quita salud; con 0 se pierde la vida, se suelta el trofeo y se reaparece con la salud llena.
- [x] Daño como dato en cada arma: pistola 20, escopeta 12 por perdigón, láser 70/s, bazuca 80 en el centro → 30 en el borde.
- [x] El golpe es un `DamageInfo` (Value Object) que `ProjectileSystem` aplica en C++; GDScript solo recibe señales finales (el láser, agrupado cada 0,25 s).
- [x] Visual: destello blanco al recibir golpe, números de daño (perdigones sumados), barrita de salud sobre rivales, borde rojo con salud < 30, sacudida con explosiones cercanas y sección SAL en la tarjeta.
- [x] Tecla de prueba 8: recibir 25 de daño. 13 tests (151 en total).

### v0.0.31 — HUD flotante ✅
- [x] Sin barra superior: pastilla del reloj (roja en el último minuto) con la línea del trofeo debajo; FPS solo si está activado.
- [x] Tarjeta compacta y flotante abajo a la izquierda (cabecera en una línea, solo el arma activa); efectos encima; se vuelve transparente si hay alguien detrás.
- [x] Objetos recogidos e info del mapa salen de la pantalla (resultados y ayuda F1).

### v0.0.30 — Arreglo: la muerte por fin se ve ✅
- [x] Causa: la vista pedía el `LifeComponent` antes de que el participante lo buscara (los hijos hacen `_ready` antes que el padre). Ahora los componentes se buscan al pedirlos.
- [x] Arregla también el parpadeo de protección al reaparecer y perder el arma especial al morir.
- [x] El shader de desintegración no compilaba (`return` en `fragment()`). Verificado con capturas.

### v0.0.29 — Tarjeta del participante ✅
- [x] Panel arriba a la izquierda: avatar con anillo de estado, píldoras (en juego, protegido, eliminado, trofeo), VEL y VIS con barra hasta el tope, efectos con anillo y armas con munición.
- [x] Composite de secciones: la salud y las vidas (Etapa 4) serán una sección más.
- [x] La barra de arriba queda para la partida (reloj, objetos, FPS); se quitó el panel de armas de abajo.

### v0.0.28 — Láser sostenido y comportamientos de disparo ✅
- [x] El láser es un rayo continuo mientras se mantiene el clic (sin parpadeo), con 4 s de batería.
- [x] Comportamientos de disparo como Strategy (`FireBehavior`): el arma recibe el gatillo apretado / mantenido / soltado.
- [x] Bots: el error de puntería se mantiene 0,25 s (el rayo barre suave).
- [x] 3 tests (138 en total).

### v0.0.27 — Muerte con desintegración ✅
- [x] Corregido: el efecto de muerte quedaba debajo del piso y no se veía (parecía que el participante se congelaba).
- [x] El cuerpo se desintegra en píxeles en su lugar (shader) mientras salen fragmentos de su color; marca en el piso.

### v0.0.26 — Armas en el mapa y HUD de armas ✅
- [x] Escopeta, láser y bazuca aparecen en el mapa como cuadrados con su símbolo (categoría "armas" con cupo, reposición y caducidad).
- [x] Al recogerla se equipa sola en la ranura especial; los bots también las recogen y las usan.
- [x] Panel de armas en el HUD (abajo al centro): pistola + especial con munición y barra.

### v0.0.25 — Catálogo de armas: escopeta, láser y bazuca ✅
- [x] Arma = combinación de datos: modo (proyectil / rayo), perdigones en cono, explosión, munición. Un arma nueva = un .tres.
- [x] Escopeta (7 perdigones en 30°, 8 tiros), láser (rayo instantáneo, 30 tiros) y bazuca (explosión de radio 2, 4 tiros).
- [x] Pistola fija + 1 ranura especial; sin munición o al ser eliminado se vuelve a la pistola.
- [x] Teclas de prueba 4/5/6/7. 5 tests (135 en total).

### v0.0.24 — Disparo más real y animación de muerte ✅
- [x] Alcance del arma = visión de quien dispara (dato `range_from_vision`); con power-up de visión se dispara más lejos.
- [x] Balas a 50 casillas/s (antes 16) con estela de bala trazadora.
- [x] Animación de eliminación (estallido + marca en el piso) y de reaparición.
- [x] 2 tests (130 en total).

### v0.0.23 — Disparo + pistola inicial ✅
- [x] Todos los participantes tienen pistola (`WeaponType` .tres: 0,35 s entre disparos, munición infinita, alcance 12 casillas).
- [x] Humano: apunta con el mouse (o stick derecho) y dispara con clic izquierdo (o gatillo); mantener = ráfaga.
- [x] Bots: disparan mientras se mueven al rival visible más cercano, con reacción y error de puntería según dificultad (`BotGunner`).
- [x] Todas las balas en un solo sistema (`ProjectileWorld`, tope 256); no atraviesan paredes aunque sean rápidas.
- [x] Impacto = eliminar (vuelve a su base y suelta el trofeo). Evento `participant_hit`.
- [x] 14 tests (128 en total).

### v0.0.22 — Lenguaje visual de objetos ✅
- [x] La forma dice la categoría (círculo = stat, rombo = power-up; reservados: cuadrado = arma, hexágono = mapa, anillo = portal).
- [x] El símbolo dice el efecto (» velocidad, ojo visión, rayo rapidez). Halo y borde para leerse sobre cualquier suelo.
- [x] Forma y símbolo como datos en `PickupType`; dibujo procedimental una sola vez.

### v0.0.21 — Economía de objetos: categorías, caducidad y topes ✅
- [x] `PickupCategory` (.tres): puntos y power-ups con cupo propio que escala con el mapa (11 y 2 en Normal) y reposición propia.
- [x] Los objetos caducan en el piso (puntos 30 s, power-ups 25 s) y parpadean antes (`ExpiryQueue`).
- [x] Valores más bajos: verde +3 VEL, amarillo +0,25 VIS. Tope de mejoras permanentes como dato: +48 VEL, +2,5 VIS ("MÁX" al llenarse).
- [x] 6 tests (114 en total).

### v0.0.20 — Atrapar al portador: eliminación y reaparición ✅
- [x] Modo de robo como dato (`TrophyRules.steal_mode`): por defecto **Eliminar al portador** (el trofeo cae al piso; el portador reaparece en su base a los 3 s, protegido 2 s). El modo Transferir sigue disponible.
- [x] `LifeCycle` (core, State) + `LifeComponent`: base de la reaparición de la Etapa 4. Vidas infinitas por ahora.
- [x] Portador a ×0,85; inmunidad al recoger 2 s; trofeo recién caído bloqueado 0,5 s.
- [x] 5 tests (108 en total).

### v0.0.19 — Anti-bucle del robo ✅
- [x] `TrophyRules` (.tres): todas las reglas del trofeo como datos; impulso al ladrón (×1,5, 2 s) y aturdimiento a la víctima (×0,2, 1 s).
- [x] Fichas de ataque (`TokenPool`): solo 2 bots persiguen al portador; los demás **emboscan** antes de su base.
- [x] 4 tests (103 en total).

### v0.0.18 — Quitar el trofeo (robo básico) y velocidad inicial pareja ✅
- [x] Robo por contacto: tocar al portador le quita el trofeo; inmunidad de 3 s al tomarlo o robarlo. Evento `trophy_stolen`.
- [x] Bots: nueva acción "quitar el trofeo" (persiguen al portador). Flecha "QUÍTASELO" y avisos en el HUD.
- [x] Todos empiezan con la misma velocidad, más lenta: 120 (antes 160); los objetos de velocidad pesan más.
- [x] 4 tests (99 en total). Reglas finas del robo (golpe, escudos anti-robo) llegan con el combate.

### v0.0.17 — Etapa 3: Bots con dificultad ✅
- [x] A* sobre celdas lógicas (con costos, listo para terrenos) y errores en cruces según la dificultad.
- [x] `BotBrain` (Utility AI): entregar, ir por el trofeo, recoger objetos VISTOS, explorar. Cada acción es una clase: agregar mecánicas = agregar acciones.
- [x] `BotProfile` .tres: Fácil, Normal, Difícil, Experto + "Mixto". `BotCoordinator`: cola de caminos (2/frame) y replanificación cuando el mapa cierra un camino.
- [x] Menú: rivales 0–7 y su dificultad. Resultado "DERROTA — ganó Bot X". Participantes se atraviesan (capas de física).
- [x] 13 tests (95 en total).

### v0.0.16 — Etapa 1.2: Partida (reloj y fases) + bases ajustadas al pasillo ✅
- [x] `MatchFlow` (core, State): Preparación → Asalto → Golpe de los vándalos → fin; victoria o tiempo agotado. 10 tests.
- [x] `MatchController` + `MatchRules` (.tres): 40/35/25, avisos a 60 y 10 s, laberinto vivo ×2 en el Golpe.
- [x] Reloj en el HUD, avisos de fase y tiempo, resultado "TIEMPO AGOTADO". Duración en el menú: 5/8/12/20 min, sin límite o personalizada (1–60).
- [x] Arreglo: la base mide el pasillo completo (antes 3×3 fijo, se metía en las paredes); el cuerpo del participante cabe en pasillos de 1. 2 tests (82 en total).
- [x] Tecla de prueba N: saltar a la siguiente fase.

### v0.0.15 — Prueba de estrés automática con registro ✅
- [x] Informe JSON + historial CSV; captura de cada tirón con el costo de cada sistema en ese frame (`PerfStats.take_frame`).
- [x] Ejecución automática (`-- --stress --quit-after-stress`) y `verificar.ps1 -Estres`.
- [x] Dirty flag en participantes: prueba de estrés 5,2 → **1,2 ms** por frame (p99 ≈ 2,3 ms). DENTRO del presupuesto.

### v0.0.14 — Escalabilidad: prioridades, índice espacial y prueba de estrés ✅
- [x] `MapChangeQueue`: prioridades (seguridad > jugador > Director > automático), reservas, tope 64, validación al aplicar.
- [x] `SpatialHash` + `EntityIndex`; `WanderController`.
- [x] Prueba de estrés en el menú principal con informe y veredicto. 7 tests (69 en total).

### v0.0.13 — Rendimiento: medición y escalabilidad ✅ (pendiente medición en vivo)
- [x] Panel de rendimiento F3 + monitores del depurador; `PerfScope` en todos los sistemas en caliente.
- [x] Cola de cambios del mapa (Command + presupuesto por frame); conectividad local acotada; caché de protegidos; muestreo; índice espacial de objetos.
- [x] `docs/RENDIMIENTO.md`: presupuesto, cómo medir, reglas. 62 tests.

### v0.0.12 — Interfaz animada, mapas grandes y laberinto vivo ✅ (en prueba en vivo)
- [x] `UiFx`: hover = selección, rebote, barra de acento y descripciones flotantes en todos los menús.
- [x] Tamaños Normal/Grande/Enorme/Gigante; Personalizado hasta 160×110. Optimización: objetos dibujados una vez.
- [x] Laberinto vivo: cambios seguros (conectividad garantizada), aviso previo, espera/cancelación si hay alguien encima, objetos y trofeo se adaptan. 7 tests.

### v0.0.11 — Etapa 1.3: Menús, flujo del juego y opciones ✅
- [x] Menú principal (fondo de laberinto animado), configurar partida con **vista previa del mapa en vivo**, pausa, resultados, opciones persistentes.
- [x] Flujo del juego con **State** (`App`), opciones (`Settings`), `MatchConfig`, tema único (`ui/theme.tres`).
- [x] Todo con mouse, teclado y mando; teclas de prueba solo si se activan en Opciones.
- [x] La escena de prueba pasó a ser la partida real (`scenes/game/match.tscn`).

### v0.0.10 — Algoritmos de generación y dificultad ✅ (adelanto de la etapa 2.1)
- [x] 6 algoritmos clásicos (Backtracking, Hunt-and-Kill, Growing Tree, Kruskal, Wilson, Prim) sobre `PassageGrid` (topología).
- [x] Bucles y *braid* (eliminar callejones); métricas: % de callejones, cruces, ruta base → centro.
- [x] 4 niveles (Fácil, Normal, Difícil, Extremo) + personalizado como datos (`MazeProfile`). Tecla M en el sandbox.
- [x] 6 tests (52 en total), incluido uno que verifica que los niveles están ordenados por dificultad.

### v0.0.9 — Etapa 1.1: Trofeo y bases ✅
- [x] `TrophyTracker` (core, State): en bóveda → llevado → en el piso → entregado. 9 tests.
- [x] `TrophyController`: lentitud ×0.6 al portador, eventos al Event Bus, vistas de trofeo (con luz) y bases.
- [x] Flecha de objetivo en el borde (trofeo / tu base) con distancia; aviso "LLEVAS EL TROFEO"; cartel de victoria.

### v0.0.8 — Bloque UX-2: HUD limpio y textos flotantes ✅
- [x] Event Bus `Events` (autoload); los niveles publican, la interfaz escucha.
- [x] HUD: barra superior delgada (stats que cuentan y destellan, fichas de efectos temporales con barra, contador); centro libre.
- [x] Textos flotantes con Object Pool (`+8 VEL`, `+0.5 VIS`, `¡RAPIDEZ! ×1.5 VEL`), visibles en la oscuridad.
- [x] Ayuda de controles en F1 (pista que se desvanece).

### v0.0.7 — Bloque UX-1: Objetos infinitos ✅
- [x] `PickupSpawner` + `PickupSpawnPolicy` (Strategy; hoy `KeepDensityPolicy`: 30 objetos, uno cada 3 s).
- [x] `pick_spawn_cell` (core): nunca aparece a la vista de un participante. 7 tests.
- [x] Animación de aparición.

### v0.0.6 — Pieza 5: Objetos recolectables ✅
- [x] Tipos de objeto como datos (`PickupType` + `PickupEffect`, `.tres`) y `PickupFactory` (Factory).
- [x] `place_pickups` (core): suelo, fuera de la bóveda, lejos de bases, separación y rareza. 5 tests.
- [x] `lv::core::Rng`: aleatorio portable (misma semilla = mismo resultado en todas las plataformas); `MazeGrid` migrado. 3 tests.
- [x] 3 tipos: punto de velocidad, punto de visión, power-up de rapidez (raro). Contador en el panel.

### v0.0.5 — Pieza 4: Visión limitada ✅
- [x] `lv::core::can_see` / `has_line_of_sight` (radio + línea de visión por casillas). 6 tests.
- [x] `Vision` (`PointLight2D` con sombras) como hijo del participante; el radio sigue a la stat `vision`.
- [x] Paredes con oclusores en su propia capa: se ven las cercanas, lo de detrás queda oculto.
- [x] Sandbox con oscuridad (tecla V la apaga).

### v0.0.4 — Pieza 3: Stats ✅
- [x] `lv::core::Stats`: valor base + pila de modificadores (suma/multiplica, temporales/permanentes, por origen, límites). 6 tests.
- [x] `StatsComponent` (hijo del participante) + `StatsProfile` (datos .tres). El participante toma su velocidad de ahí.
- [x] Panel de stats arriba a la izquierda (Observer de las señales). Teclas 1/2/3/0 para probar efectos.

### v0.0.3 — Laberinto grande + Módulo 2: Participante ✅
- [x] Pasillos anchos (3 casillas) con paredes de 1; laberinto de 20×14 celdas (81×57 casillas). +1 test.
- [x] `Participant` (`CharacterBody2D`) con controlador intercambiable (**Strategy**) que produce órdenes (**Command**).
- [x] `HumanController`: WASD y flechas.
- [x] Vista provisional separada de la lógica; escena de prueba con cámara que sigue al participante (Tab = mapa completo).

### v0.0.2 — Módulo 1: Laberinto ✅
- [x] `lv::core::MazeGrid`: generación por semilla (backtracking + bucles + bóveda central), bases, conectividad.
- [x] 9 tests unitarios.
- [x] Nodo `Maze` (`TileMapLayer`) con TileSet provisional de colores y colisión en las paredes.
- [x] Escena de prueba `scenes/sandbox/maze_sandbox.tscn` (R = otro laberinto).

### v0.0.1 — Base del proyecto ✅
- [x] Entorno de desarrollo (Godot 4.7, godot-cpp 10, SCons, MSVC, VS Code).
- [x] Proyecto creado desde godot-cpp-template; la extensión se llama `laberinto`.
- [x] Tests unitarios con doctest (`scons tests=yes`).
- [x] Documentación: README, CLAUDE.md, VISION, CONTRIBUTING, CREDITS, LICENSE.
- [x] CI en GitHub Actions (tests + build Linux/Windows).

### Diseño de arquitectura ✅
- [x] [`ARQUITECTURA.md`](ARQUITECTURA.md): capas `core`/`game`, mapa de módulos de la fase 1 y patrones.
