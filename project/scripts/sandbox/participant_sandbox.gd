extends Node2D
## Escena de prueba de la fase 1 (participante, stats, visión, objetos, HUD, trofeo y bases).
## No es el juego final: la partida (reloj, fases) y las pantallas llegan en las piezas siguientes.
## Las teclas están en el panel de controles (F1).

const FOLLOW_ZOOM := Vector2(2.0, 2.0)
const MAP_PROFILES: Array[MazeProfile] = [
	preload("res://data/maps/facil.tres"),
	preload("res://data/maps/normal.tres"),
	preload("res://data/maps/dificil.tres"),
	preload("res://data/maps/extremo.tres"),
]
const DEBUG_SOURCE := "prueba"
const CONTROLS := """Objetivo: toma el TROFEO del centro y llévalo a TU BASE.
La flecha del borde te indica a dónde ir.

WASD / flechas · moverse
F1 · mostrar u ocultar esta ayuda

Pruebas
1 · power-up ×1.5 VEL (5 s)
2 · +10 VEL permanente
3 · +2 VIS (5 s)
0 · quitar efectos de prueba
T · soltar el trofeo
V · niebla sí / no
Tab · ver el mapa completo
R · otro laberinto (nueva partida)
M · cambiar dificultad del mapa
Esc · salir"""

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
var _profile_index := 1  # Normal


func _ready() -> void:
	_participant.set_controller(HumanController.new())
	_hud.set_controls_text(CONTROLS)
	_hud.bind(_participant)
	# Publicar en el Event Bus lo que ocurre en este nivel; la interfaz escucha ahí.
	_pickups.pickup_collected.connect(func(pickup: Pickup, participant: Participant) -> void:
		Events.pickup_collected.emit(pickup.type, participant, pickup.global_position))
	_spawner.pickup_spawned.connect(func(pickup: Pickup) -> void: Events.pickup_spawned.emit(pickup))
	_trophy.trophy_taken.connect(func(p: Participant) -> void: Events.trophy_taken.emit(p))
	_trophy.trophy_dropped.connect(func(p: Participant, pos: Vector2) -> void: Events.trophy_dropped.emit(p, pos))
	_trophy.trophy_delivered.connect(func(p: Participant) -> void: Events.trophy_delivered.emit(p))
	Events.trophy_delivered.connect(_on_trophy_delivered)
	_hud.bind_objective(_trophy, _maze)
	_maze.generated.connect(_on_maze_generated)
	if _maze.get_last_seed() != 0:
		_on_maze_generated(_maze.get_last_seed())


func _process(delta: float) -> void:
	if not _finished:
		_elapsed += delta
	if not _overview:
		_camera.position = _participant.position


func _unhandled_input(event: InputEvent) -> void:
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
		KEY_R:
			_maze.generate()
		KEY_M:
			_profile_index = (_profile_index + 1) % MAP_PROFILES.size()
			_maze.profile = MAP_PROFILES[_profile_index]
			_maze.generate()
		KEY_TAB:
			_overview = not _overview
			_update_camera()
		KEY_ESCAPE:
			get_tree().quit()


func _on_maze_generated(seed_value: int) -> void:
	# La cantidad de objetos depende del tamaño del mapa (perfil).
	var pickup_count: int = _maze.profile.get_pickup_count() if _maze.profile else 30
	_pickups.count = pickup_count
	(_spawner.policy as KeepDensityPolicy).target_count = pickup_count
	var m: Dictionary = _maze.get_metrics()
	var size: Vector2i = _maze.get_grid_size()
	_hud.set_map_info("MAPA %s · %s · %d×%d · callejones %d%% · ruta %d · [M] cambiar" % [
		_maze.profile.get_display_name().to_upper() if _maze.profile else "?", m.algorithm, size.x, size.y,
		roundi(m.dead_end_ratio * 100.0), m.route_length])
	# Objetos infinitos: el spawner coloca los iniciales y repone los recogidos (fuera de la vista).
	# Misma semilla que el laberinto (determinista). Se dejan libres las 4 esquinas (bases).
	_spawner.start(_maze, _maze.get_base_cells(4), seed_value)
	_hud.reset_collected()
	_hud.hide_banner()
	var base: Vector2i = _maze.get_base_cells(1)[0]
	_participant.position = _maze.cell_to_world(base)
	# Trofeo y bases: participante i -> base i.
	var participants: Array[Participant] = [_participant]
	_trophy.start(_maze, participants)
	_participant.set_controller(HumanController.new())
	_elapsed = 0.0
	_finished = false
	_camera.position = _participant.position
	_camera.reset_smoothing()
	_update_camera()


func _update_camera() -> void:
	if not _overview:
		_camera.zoom = FOLLOW_ZOOM
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


func _on_trophy_delivered(participant: Participant) -> void:
	_finished = true
	participant.set_controller(null)  # quieto: terminó la partida
	var time := "%d:%02d" % [int(_elapsed) / 60, int(_elapsed) % 60]
	_hud.show_banner("¡VICTORIA!", "Llevaste el trofeo a tu base en %s\nR · jugar otra vez" % time, Color(1.0, 0.82, 0.25))