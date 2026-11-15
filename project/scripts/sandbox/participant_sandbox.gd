extends Node2D
## Escena de prueba de las piezas 2 a 5 (participante, stats, visión, objetos). No es parte del juego final.
## WASD/flechas = moverse · 1/2/3 = efectos de prueba · 0 = quitar efectos
## V = niebla on/off · Tab = ver mapa completo · R = otro laberinto · Esc = salir

const FOLLOW_ZOOM := Vector2(2.0, 2.0)
const DEBUG_SOURCE := "prueba"

@onready var _maze: Maze = $Maze
@onready var _participant: Participant = $Participant
@onready var _camera: Camera2D = $Camera
@onready var _help: Label = $UI/Help
@onready var _darkness: CanvasModulate = $Darkness
@onready var _pickups: PickupFactory = $Pickups
@onready var _spawner: PickupSpawner = $PickupSpawner
@onready var _panel = $UI/StatsPanel

var _collected := 0

var _overview := false


func _ready() -> void:
	_participant.set_controller(HumanController.new())
	_panel.bind(_participant)
	_pickups.pickup_collected.connect(_on_pickup_collected)
	_maze.generated.connect(_on_maze_generated)
	if _maze.get_last_seed() != 0:
		_on_maze_generated(_maze.get_last_seed())
	_help.text = "[WASD/flechas] moverse   [1] power-up ×1.5 vel 5s   [2] punto +10 vel   [3] +2 visión 5s   [0] quitar efectos   [V] niebla   [Tab] mapa   [R] otro laberinto   [Esc] salir"


func _process(_delta: float) -> void:
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
		KEY_V:
			_darkness.visible = not _darkness.visible
		KEY_R:
			_maze.generate()
		KEY_TAB:
			_overview = not _overview
			_update_camera()
		KEY_ESCAPE:
			get_tree().quit()


func _on_maze_generated(seed_value: int) -> void:
	# Objetos infinitos: el spawner coloca los iniciales y repone los recogidos (fuera de la vista).
	# Misma semilla que el laberinto (determinista). Se dejan libres las 4 esquinas (bases).
	_spawner.start(_maze, _maze.get_base_cells(4), seed_value)
	print("[sandbox] objetos colocados: ", _pickups.get_active_count())
	_collected = 0
	_panel.set_collected(_collected)
	var base: Vector2i = _maze.get_base_cells(1)[0]
	_participant.position = _maze.cell_to_world(base)
	_camera.position = _participant.position
	_camera.reset_smoothing()
	_update_camera()


func _update_camera() -> void:
	if not _overview:
		_camera.zoom = FOLLOW_ZOOM
		return
	var rect := _maze.get_used_rect()
	var tile := Vector2(_maze.tile_set.tile_size)
	var world_size := Vector2(rect.size) * tile
	_camera.position = Vector2(rect.position) * tile + world_size / 2.0
	var view := get_viewport_rect().size * 0.95
	var z := minf(view.x / world_size.x, view.y / world_size.y)
	_camera.zoom = Vector2(z, z)


func _on_pickup_collected(pickup: Pickup, _participant: Participant) -> void:
	_collected += 1
	_panel.set_collected(_collected)
	print("[sandbox] recogido: ", pickup.type.display_name)