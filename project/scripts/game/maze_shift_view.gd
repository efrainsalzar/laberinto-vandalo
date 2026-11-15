class_name MazeShiftView
extends Node2D
## Vista del laberinto vivo. Solo dibuja; la lógica está en MazeShifter (C++).
## - Aviso: las casillas que se van a CERRAR parpadean en rojo (cada vez más rápido); las que se van a
##   ABRIR, con un contorno celeste. Así nadie es sorprendido (VISION §10: ataques justos).
## - Al aplicarse: destello breve. Si un cierre está bloqueado (alguien encima), parpadea más rápido.
## Solo se redibuja mientras hay algo que mostrar (rendimiento). Ver docs/ARQUITECTURA.md §15.

const CLOSE_COLOR := Color(1.0, 0.3, 0.25)
const OPEN_COLOR := Color(0.35, 0.85, 1.0)
const FLASH_TIME := 0.35

var _maze: Maze
var _closing := {}  # Vector2i -> true
var _opening := {}
var _blocked := {}
var _flashes: Array[Dictionary] = []  # {tiles, color, age}
var _warn_left := 0.0
var _warn_total := 1.0
var _time := 0.0


func bind(maze: Maze, shifter: MazeShifter) -> void:
	_maze = maze
	z_index = 4
	shifter.shift_warned.connect(_on_warned)
	shifter.closure_blocked.connect(func(tiles: Array) -> void:
		for t in tiles: _blocked[t] = true)
	shifter.closure_cancelled.connect(func(tiles: Array) -> void:
		for t in tiles:
			_closing.erase(t)
			_blocked.erase(t)
		_flashes.append({"tiles": tiles, "color": OPEN_COLOR, "age": 0.0}))
	maze.map_changed.connect(_on_map_changed)
	maze.generated.connect(func(_s: int) -> void: _clear())


func _clear() -> void:
	_closing.clear()
	_opening.clear()
	_blocked.clear()
	_flashes.clear()
	queue_redraw()


func _on_warned(closing: Array, opening: Array, seconds: float) -> void:
	for t in closing: _closing[t] = true
	for t in opening: _opening[t] = true
	_warn_left = seconds
	_warn_total = maxf(seconds, 0.01)


func _on_map_changed(tiles: Array, now_wall: bool) -> void:
	for t in tiles:
		_closing.erase(t)
		_opening.erase(t)
		_blocked.erase(t)
	_flashes.append({"tiles": tiles, "color": CLOSE_COLOR if now_wall else OPEN_COLOR, "age": 0.0})


func _process(delta: float) -> void:
	if _closing.is_empty() and _opening.is_empty() and _flashes.is_empty():
		return
	_time += delta
	_warn_left = maxf(_warn_left - delta, 0.0)
	for f in _flashes:
		f.age += delta
	_flashes = _flashes.filter(func(f: Dictionary) -> bool: return f.age < FLASH_TIME)
	queue_redraw()


func _draw() -> void:
	if _maze == null:
		return
	var tile := _maze.get_tile_px()
	var half := Vector2(tile, tile) / 2.0
	# Parpadeo que se acelera al acercarse el cambio.
	var urgency := 1.0 - _warn_left / _warn_total
	var blink := 0.5 + 0.5 * sin(_time * lerpf(6.0, 22.0, urgency))
	for t in _closing:
		var rect := Rect2(to_local(_maze.cell_to_world(t)) - half, Vector2(tile, tile))
		var speed := 30.0 if _blocked.has(t) else 1.0
		var a := (0.25 + 0.45 * blink) if speed == 1.0 else 0.35 + 0.45 * (0.5 + 0.5 * sin(_time * speed))
		draw_rect(rect, Color(CLOSE_COLOR, a))
		draw_rect(rect.grow(-1.0), Color(CLOSE_COLOR, 0.9), false, 1.5)
	for t in _opening:
		var rect := Rect2(to_local(_maze.cell_to_world(t)) - half, Vector2(tile, tile))
		draw_rect(rect.grow(-1.5), Color(OPEN_COLOR, 0.35 + 0.5 * blink), false, 2.0)
	for f in _flashes:
		var k: float = 1.0 - f.age / FLASH_TIME
		for t in f.tiles:
			var rect := Rect2(to_local(_maze.cell_to_world(t)) - half, Vector2(tile, tile)).grow(3.0 * k)
			draw_rect(rect, Color(Color.WHITE.lerp(f.color, 0.4), 0.7 * k))
