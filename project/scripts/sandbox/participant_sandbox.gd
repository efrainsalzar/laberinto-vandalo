extends Node2D
## Escena de prueba de la fase 1 (participante, stats, visión, objetos, HUD). No es el juego final.
## Las teclas están en el panel de controles (F1).

const FOLLOW_ZOOM := Vector2(2.0, 2.0)
const DEBUG_SOURCE := "prueba"
const CONTROLS := """WASD / flechas · moverse
F1 · mostrar u ocultar esta ayuda

Pruebas
1 · power-up ×1.5 VEL (5 s)
2 · +10 VEL permanente
3 · +2 VIS (5 s)
0 · quitar efectos de prueba
V · niebla sí / no
Tab · ver el mapa completo
R · otro laberinto
Esc · salir"""

@onready var _maze: Maze = $Maze
@onready var _participant: Participant = $Participant
@onready var _camera: Camera2D = $Camera
@onready var _darkness: CanvasModulate = $Darkness
@onready var _pickups: PickupFactory = $Pickups
@onready var _spawner: PickupSpawner = $PickupSpawner
@onready var _hud: HUD = $HUD

var _overview := false


func _ready() -> void:
	_participant.set_controller(HumanController.new())
	_hud.set_controls_text(CONTROLS)
	_hud.bind(_participant)
	# Publicar en el Event Bus lo que ocurre en este nivel; la interfaz escucha ahí.
	_pickups.pickup_collected.connect(func(pickup: Pickup, participant: Participant) -> void:
		Events.pickup_collected.emit(pickup.type, participant, pickup.global_position))
	_spawner.pickup_spawned.connect(func(pickup: Pickup) -> void: Events.pickup_spawned.emit(pickup))
	_maze.generated.connect(_on_maze_generated)
	if _maze.get_last_seed() != 0:
		_on_maze_generated(_maze.get_last_seed())


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
	_hud.reset_collected()
	var base: Vector2i = _maze.get_base_cells(1)[0]
	_participant.position = _maze.cell_to_world(base)
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
