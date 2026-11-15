extends Node2D
## Una partida: arma el nivel con la configuración elegida en el menú (App.match_config) y conecta
## los sistemas. Pausa con Esc/P (botón Start en mando), resultados al ganar.
## La partida (reloj, fases, fin por tiempo) llega en la etapa 1.2.
## Ver docs/ARQUITECTURA.md §13.

const DEBUG_SOURCE := "prueba"
const CONTROLS := """Objetivo: toma el TROFEO del centro y llévalo a TU BASE.
La flecha del borde te indica a dónde ir.

WASD / flechas · moverse
Esc / P · pausa
F1 · mostrar u ocultar esta ayuda"""
const DEBUG_CONTROLS := """

Teclas de prueba (Opciones › Teclas de prueba)
1 · power-up ×1.5 VEL (5 s)   2 · +10 VEL
3 · +2 VIS (5 s)   0 · quitar efectos
T · soltar el trofeo   V · niebla sí / no
Tab · ver el mapa completo"""
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

var _overview := false
var _elapsed := 0.0
var _finished := false
var _pause_menu: CanvasLayer


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
	_trophy.trophy_dropped.connect(func(p: Participant, pos: Vector2) -> void: Events.trophy_dropped.emit(p, pos))
	_trophy.trophy_delivered.connect(func(p: Participant) -> void: Events.trophy_delivered.emit(p))
	Events.trophy_delivered.connect(_on_trophy_delivered)
	_maze.generated.connect(_on_maze_generated)
	# Configuración elegida en el menú.
	var config := App.match_config
	if config.profile:
		_maze.profile = config.profile
	_maze.seed = config.seed
	_maze.generate()


func _process(delta: float) -> void:
	if not _finished:
		_elapsed += delta
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
	var base: Vector2i = _maze.get_base_cells(1)[0]
	_participant.position = _maze.cell_to_world(base)
	var participants: Array[Participant] = [_participant]
	_trophy.start(_maze, participants)
	_participant.set_controller(HumanController.new())
	_elapsed = 0.0
	_finished = false
	_camera.position = _participant.position
	_camera.reset_smoothing()
	_update_camera()


func _on_trophy_delivered(participant: Participant) -> void:
	_finished = true
	participant.set_controller(null)  # quieto: terminó la partida
	App.show_results()
	var results := RESULTS_SCREEN.instantiate()
	add_child(results)
	var time := "%d:%02d" % [int(_elapsed) / 60, int(_elapsed) % 60]
	var collected: String = _hud.get_node("TopBar/Row/Collected").text
	results.show_result("¡VICTORIA!", Color(1.0, 0.82, 0.25),
		"Llevaste el trofeo a tu base en %s\n%s · mapa %s · semilla %d" % [
			time, collected, _maze.profile.get_display_name() if _maze.profile else "", _maze.get_last_seed()])


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
