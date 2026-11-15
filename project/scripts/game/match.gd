extends Node2D
## Una partida: arma el nivel con la configuración elegida en el menú (App.match_config) y conecta
## los sistemas. Pausa con Esc/P (botón Start en mando), resultados al ganar o al acabarse el tiempo.
## Reloj y fases: MatchController (§18). Ver docs/ARQUITECTURA.md §13.

const DEBUG_SOURCE := "prueba"
const CONTROLS := """Objetivo: toma el TROFEO del centro y llévalo a TU BASE
antes de que se acabe el tiempo (si no, los vándalos se lo llevan).
La flecha del borde te indica a dónde ir.
Si otro lleva el trofeo, TÓCALO para quitárselo (quien lo toma es inmune unos segundos).
Fases: Preparación → Asalto → Golpe de los vándalos (el laberinto se acelera).

WASD / flechas · moverse
Esc / P · pausa
F1 · mostrar u ocultar esta ayuda"""
const DEBUG_CONTROLS := """

Teclas de prueba (Opciones › Teclas de prueba)
1 · power-up ×1.5 VEL (5 s)   2 · +10 VEL
3 · +2 VIS (5 s)   0 · quitar efectos
T · soltar el trofeo   V · niebla sí / no
K · mover el laberinto ya   Tab · ver el mapa completo
N · saltar a la siguiente fase"""
const PARTICIPANT_SCENE := preload("res://scenes/participant/participant.tscn")
const BOT_NAMES := ["Rojo", "Verde", "Naranja", "Violeta", "Rosa", "Turquesa", "Blanco"]
const BOT_COLORS := [
	Color(1.0, 0.4, 0.35), Color(0.45, 0.9, 0.45), Color(1.0, 0.68, 0.25), Color(0.75, 0.5, 1.0),
	Color(1.0, 0.55, 0.8), Color(0.35, 0.9, 0.85), Color(0.92, 0.92, 0.92),
]
const PAUSE_MENU := preload("res://scenes/menus/pause_menu.tscn")
const RESULTS_SCREEN := preload("res://scenes/menus/results_screen.tscn")

@onready var _maze: Maze = $Maze
@onready var _participant: Participant = $Participant
@onready var _camera: Camera2D = $Camera
@onready var _darkness: CanvasModulate = $Darkness
@onready var _pickups: PickupFactory = $Pickups
@onready var _spawner: PickupSpawner = $PickupSpawner
@onready var _hud: HUD = $HUD
@onready var _trophy: TrophyController = $Trophy
@onready var _shifter: MazeShifter = $MazeShifter
@onready var _shift_view: MazeShiftView = $ShiftView
@onready var _match: MatchController = $MatchController
@onready var _bots: BotCoordinator = $Bots

## Radio normal del cuerpo del participante (px) y cuánto del pasillo puede ocupar como máximo.
const BODY_RADIUS := 11.0
const BODY_MAX_CORRIDOR_SHARE := 0.4

var _overview := false
var _finished := false
var _live_interval := 0.0  # intervalo del laberinto vivo elegido en el menú (antes del ritmo de la fase)
var _pause_menu: CanvasLayer
var _stress: Node
var _bot_nodes: Array[Participant] = []


func _ready() -> void:
	_hud.bind(_participant)
	_hud.bind_objective(_trophy, _maze)
	_update_controls_text()
	Settings.changed.connect(_on_setting_changed)
	# Publicar en el Event Bus lo que ocurre en este nivel; la interfaz escucha ahí.
	_pickups.pickup_collected.connect(func(pickup: Pickup, participant: Participant) -> void:
		Events.pickup_collected.emit(pickup.type, participant, pickup.global_position))
	_spawner.pickup_spawned.connect(func(pickup: Pickup) -> void: Events.pickup_spawned.emit(pickup))
	_trophy.trophy_taken.connect(func(p: Participant) -> void: Events.trophy_taken.emit(p))
	_trophy.trophy_stolen.connect(func(thief: Participant, victim: Participant) -> void: Events.trophy_stolen.emit(thief, victim))
	Events.trophy_stolen.connect(_on_trophy_stolen)
	Events.trophy_taken.connect(func(p: Participant) -> void:
		if p != _participant:
			_hud.toast("¡%s TOMÓ EL TROFEO!" % p.display_name.to_upper(), p.color, 2.5))
	_trophy.trophy_dropped.connect(func(p: Participant, pos: Vector2) -> void: Events.trophy_dropped.emit(p, pos))
	_trophy.trophy_delivered.connect(func(p: Participant) -> void: Events.trophy_delivered.emit(p))
	# La partida: quien entrega el trofeo gana; el reloj y las fases se publican en el Event Bus.
	Events.trophy_delivered.connect(func(p: Participant) -> void: _match.declare_winner(p))
	_match.phase_changed.connect(func(phase: int, phase_name: String) -> void: Events.match_phase_changed.emit(phase, phase_name))
	_match.time_warning.connect(func(seconds_left: float) -> void: Events.match_time_warning.emit(seconds_left))
	_match.match_ended.connect(func(outcome: int, winner: Object) -> void: Events.match_ended.emit(outcome, winner as Participant))
	Events.match_phase_changed.connect(_on_phase_changed.unbind(1))
	Events.match_ended.connect(_on_match_ended)
	_hud.bind_match(_match)
	_shifter.shift_warned.connect(func(c: Array, o: Array, s: float) -> void: Events.maze_shift_warned.emit(c, o, s))
	_maze.map_changed.connect(func(tiles: Array, now_wall: bool) -> void: Events.maze_changed.emit(tiles, now_wall))
	_shift_view.bind(_maze, _shifter)
	_maze.generated.connect(_on_maze_generated)
	# Configuración elegida en el menú.
	var config := App.match_config
	if config.profile:
		_maze.profile = config.profile
	_maze.seed = config.seed
	_maze.generate()


func _process(_delta: float) -> void:
	if not _overview:
		_camera.position = _participant.position


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("pause") and not _finished:
		_open_pause()
		get_viewport().set_input_as_handled()
		return
	if not Settings.get_value("debug_keys"):
		return
	if not (event is InputEventKey and event.pressed and not event.echo):
		return
	var stats: StatsComponent = _participant.get_stats()
	match event.keycode:
		KEY_1:
			stats.add_modifier(StatsComponent.STAT_SPEED, StatsComponent.OP_MULTIPLY, 1.5, 5.0, DEBUG_SOURCE)
		KEY_2:
			stats.add_modifier(StatsComponent.STAT_SPEED, StatsComponent.OP_ADD, 10.0, -1.0, DEBUG_SOURCE)
		KEY_3:
			stats.add_modifier(StatsComponent.STAT_VISION, StatsComponent.OP_ADD, 2.0, 5.0, DEBUG_SOURCE)
		KEY_0:
			stats.remove_modifiers_from(DEBUG_SOURCE)
		KEY_T:
			_trophy.drop_trophy()
		KEY_K:
			_shifter.shift_now()
		KEY_N:
			_match.skip_phase()
		KEY_V:
			_darkness.visible = not _darkness.visible
		KEY_TAB:
			_overview = not _overview
			_update_camera()


func _on_maze_generated(seed_value: int) -> void:
	# Guardar la semilla real: "Reiniciar" y "Jugar otra vez" repiten este mismo mapa.
	App.match_config.seed = seed_value
	var pickup_count: int = _maze.profile.get_pickup_count() if _maze.profile else 30
	_pickups.count = pickup_count
	(_spawner.policy as KeepDensityPolicy).target_count = pickup_count
	var m: Dictionary = _maze.get_metrics()
	var size: Vector2i = _maze.get_grid_size()
	_hud.set_map_info("MAPA %s · %s · %d×%d · semilla %d" % [
		_maze.profile.get_display_name().to_upper() if _maze.profile else "?", m.algorithm, size.x, size.y, seed_value])
	_spawner.start(_maze, _maze.get_base_cells(4), seed_value)
	_hud.reset_collected()
	# La zona de resguardo mide lo que mide el pasillo: cada participante empieza en el centro de la suya y
	# su cuerpo se ajusta para caber siempre (en pasillos de 1 casilla, el cuerpo normal no pasaría).
	var bot_count := clampi(App.match_config.bot_count, 0, BotCoordinator.MAX_BOTS)
	var zones: Array[Rect2i] = _maze.get_base_zones(1 + bot_count)
	bot_count = mini(bot_count, zones.size() - 1)  # hay como máximo 8 bases
	var zone_world: Rect2 = _maze.tile_rect_to_world(zones[0])
	var body_radius := minf(BODY_RADIUS, zone_world.size.x * BODY_MAX_CORRIDOR_SHARE)
	_participant.body_radius = body_radius
	_participant.position = zone_world.get_center()
	_participant.set_controller(HumanController.new())
	var participants: Array[Participant] = [_participant]
	participants.append_array(_spawn_bots(bot_count, zones, body_radius, seed_value))
	_trophy.start(_maze, participants)
	_match.start(App.match_config.duration_minutes * 60.0)
	_configure_live_maze(seed_value)
	if App.match_config.stress_test and _stress == null:
		_stress = preload("res://scripts/game/stress_test.gd").new()
		add_child(_stress)
		_stress.setup(self, _maze, _hud, App.match_config)
		_stress.start()
	_finished = false
	_camera.position = _participant.position
	_camera.reset_smoothing()
	_update_camera()


## Crea los rivales: cada uno en su base, con su color, su perfil de dificultad y un BotController.
func _spawn_bots(count: int, zones: Array[Rect2i], body_radius: float, seed_value: int) -> Array[Participant]:
	for old in _bot_nodes:
		old.queue_free()
	_bot_nodes.clear()
	_bots.setup(_maze, _trophy, _pickups)
	var rng := RandomNumberGenerator.new()
	rng.seed = seed_value
	for i in count:
		var bot: Participant = PARTICIPANT_SCENE.instantiate()
		bot.name = "Bot%s" % BOT_NAMES[i]
		bot.display_name = "Bot %s" % BOT_NAMES[i]
		bot.color = BOT_COLORS[i]
		bot.participant_id = i + 1
		# Sin luz propia: los rivales no te iluminan el mapa (y ahorra sombras). Su stat de visión sigue
		# sirviéndole al bot para ver objetos (BotCoordinator).
		bot.get_node("Vision").free()
		_participant.add_sibling(bot)
		bot.body_radius = body_radius
		bot.position = _maze.tile_rect_to_world(zones[i + 1]).get_center()
		var level: int = App.match_config.bot_difficulty
		if level == MatchConfig.BOT_MIXED:
			level = rng.randi_range(0, MatchConfig.BOT_PROFILES.size() - 1)
		var profile: BotProfile = load(MatchConfig.BOT_PROFILES[clampi(level, 0, MatchConfig.BOT_PROFILES.size() - 1)])
		_bots.add_bot(bot, profile, zones[i + 1], seed_value + (i + 1) * 7919)
		_bot_nodes.append(bot)
	return _bot_nodes


## Laberinto vivo según la configuración: ritmo por nivel y más cambios simultáneos en mapas grandes.
func _configure_live_maze(seed_value: int) -> void:
	var level: Array = MatchConfig.LIVE_MAZE_LEVELS[App.match_config.live_maze]
	_shifter.enabled = level[1] > 0
	if not _shifter.enabled:
		_shifter.stop()
		return
	var scale: float = _maze.profile.size_scale if _maze.profile else 1.0
	_live_interval = level[0]
	_shifter.interval = _live_interval / maxf(0.1, _match.get_maze_speed())
	_shifter.pairs = int(level[1] * maxf(1.0, scale))
	_shifter.start(_maze, seed_value)


func _on_trophy_stolen(thief: Participant, victim: Participant) -> void:
	var seconds: float = _trophy.get_steal_immunity()
	if thief == _participant:
		_hud.toast("¡LE QUITASTE EL TROFEO A %s! · %d s sin que te lo quiten" % [victim.display_name.to_upper(), roundi(seconds)],
			Color(1.0, 0.82, 0.25), 3.0)
	elif victim == _participant:
		_hud.toast("¡%s TE QUITÓ EL TROFEO! · tócalo para recuperarlo" % thief.display_name.to_upper(), thief.color, 3.0)
	else:
		_hud.toast("%s le quitó el trofeo a %s" % [thief.display_name.to_upper(), victim.display_name.to_upper()], thief.color, 2.0)


## Cada fase cambia el ritmo del laberinto vivo (datos: data/match/default_rules.tres).
func _on_phase_changed(_phase: int) -> void:
	if _shifter.enabled and _live_interval > 0.0:
		_shifter.interval = _live_interval / maxf(0.1, _match.get_maze_speed())


func _on_match_ended(outcome: int, winner: Participant) -> void:
	if App.match_config.stress_test:
		return  # la prueba de estrés sigue midiendo y muestra su propio informe
	_finished = true
	_shifter.stop()
	for p in get_tree().get_nodes_in_group("participants"):
		(p as Participant).set_controller(null)  # quietos: terminó la partida
	App.show_results()
	var results := RESULTS_SCREEN.instantiate()
	add_child(results)
	var elapsed := _match.get_elapsed()
	var time := "%d:%02d" % [int(elapsed) / 60, int(elapsed) % 60]
	var collected: String = _hud.get_node("TopBar/Row/Collected").text
	var map := "mapa %s · semilla %d" % [_maze.profile.get_display_name() if _maze.profile else "", _maze.get_last_seed()]
	if outcome == MatchController.OUTCOME_VICTORY and winner == _participant:
		results.show_result("¡VICTORIA!", Color(1.0, 0.82, 0.25),
			"Llevaste el trofeo a tu base en %s\n%s · %s" % [time, collected, map])
	elif outcome == MatchController.OUTCOME_VICTORY and winner != null:
		results.show_result("DERROTA", winner.color,
			"Ganó %s: llevó el trofeo a su base en %s\n%s · %s" % [winner.display_name, time, collected, map])
	else:
		results.show_result("TIEMPO AGOTADO", Color(1.0, 0.4, 0.35),
			"Los vándalos se llevan el trofeo. Nadie gana.\n%s · %s" % [collected, map])


func _open_pause() -> void:
	if _pause_menu != null:
		return
	App.pause()
	_pause_menu = PAUSE_MENU.instantiate()
	add_child(_pause_menu)
	_pause_menu.resumed.connect(func() -> void: _pause_menu = null)


func _on_setting_changed(key: String, _value: Variant) -> void:
	match key:
		"camera_zoom":
			_update_camera()
		"debug_keys":
			_update_controls_text()
			if not Settings.get_value("debug_keys"):
				_darkness.visible = true
				_overview = false
				_update_camera()


func _update_controls_text() -> void:
	_hud.set_controls_text(CONTROLS + (DEBUG_CONTROLS if Settings.get_value("debug_keys") else ""))


func _update_camera() -> void:
	if not _overview:
		var z: float = Settings.get_value("camera_zoom")
		_camera.zoom = Vector2(z, z)
		_camera.offset = Vector2.ZERO
		return
	var rect := _maze.get_used_rect()
	var tile := Vector2(_maze.tile_set.tile_size)
	var world_size := Vector2(rect.size) * tile
	_camera.position = Vector2(rect.position) * tile + world_size / 2.0
	# Se deja libre la barra superior del HUD.
	var view := (get_viewport_rect().size - Vector2(0, 40)) * 0.95
	var z := minf(view.x / world_size.x, view.y / world_size.y)
	_camera.zoom = Vector2(z, z)
	_camera.offset = Vector2(0, -20) / z
