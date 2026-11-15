extends Node2D
## Escena de prueba del módulo 1 (laberinto). No es parte del juego final.
## R = generar otro laberinto · Esc = salir

@onready var _maze: Maze = $Maze
@onready var _camera: Camera2D = $Camera
@onready var _info: Label = $UI/Info


func _ready() -> void:
	_maze.generated.connect(_on_maze_generated)
	if _maze.get_last_seed() != 0:
		_on_maze_generated(_maze.get_last_seed())


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		match event.keycode:
			KEY_R:
				_maze.generate()
			KEY_ESCAPE:
				get_tree().quit()


func _on_maze_generated(seed_value: int) -> void:
	_fit_camera()
	var size: Vector2i = _maze.get_grid_size()
	_info.text = "Laberinto %dx%d · semilla %d · bases: %s · centro: %s   [R] otro  [Esc] salir" % [
		size.x, size.y, seed_value, _maze.get_base_cells(4), _maze.get_center_cell()]
	print("[sandbox] ", _info.text)


func _fit_camera() -> void:
	var rect := _maze.get_used_rect()
	var tile := Vector2(_maze.tile_set.tile_size)
	var world_size := Vector2(rect.size) * tile
	_camera.position = Vector2(rect.position) * tile + world_size / 2.0
	var view := get_viewport_rect().size * 0.92
	var z := minf(view.x / world_size.x, view.y / world_size.y)
	_camera.zoom = Vector2(z, z)
