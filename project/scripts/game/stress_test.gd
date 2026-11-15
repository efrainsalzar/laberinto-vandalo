extends Node
## Prueba de estrés (regla 10 de docs/RENDIMIENTO.md). La crea la partida si App.match_config.stress_test.
##
## Carga: N entidades que deambulan (WanderController), laberinto vivo Caótico en mapa Gigante, y ráfagas de
## cambios de dos fuentes falsas ("jugador" y "Director") que compiten con el laberinto vivo por los mismos
## pasajes (prioridades y reservas). VSync se apaga durante la prueba para medir el costo real.
## Al terminar muestra un informe: promedio, percentiles 95/99, peor frame, sistemas más caros y veredicto.

const PARTICIPANT_SCENE := preload("res://scenes/participant/participant.tscn")
const RESULTS_SCREEN := preload("res://scenes/menus/results_screen.tscn")
const SOURCE_PLAYER := 1001
const SOURCE_DIRECTOR := 1002
const WARMUP := 2.0  # segundos de calentamiento excluidos (carga, primeras compilaciones de shaders)
const GOOD_MS := 1000.0 / 60.0
const BAD_MS := 1000.0 / 30.0

var _match: Node
var _maze: Maze
var _hud: HUD
var _config: MatchConfig
var _frames: PackedFloat32Array = []
var _systems := {}  # nombre -> {sum_ms, max_ms, windows}
var _elapsed := 0.0
var _burst_timer := 0.0
var _director_timer := 0.0
var _running := false
var _worst_at := 0.0  # segundo de la prueba en que ocurrió el peor frame
var _worst_ms := 0.0
var _previous_vsync := DisplayServer.VSYNC_ENABLED


func setup(match_node: Node, maze: Maze, hud: HUD, config: MatchConfig) -> void:
	_match = match_node
	_maze = maze
	_hud = hud
	_config = config


func start() -> void:
	_spawn_wanderers(_config.stress_entities)
	_frames.clear()
	_systems.clear()
	_elapsed = 0.0
	_running = true
	if DisplayServer.get_name() != "headless":
		_previous_vsync = DisplayServer.window_get_vsync_mode()
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	Perf.window_closed.connect(_on_window)
	_hud.toast("PRUEBA DE ESTRÉS: %d s" % int(_config.stress_seconds), Color(1.0, 0.82, 0.25), 2.5)


func _exit_tree() -> void:
	if DisplayServer.get_name() != "headless":
		DisplayServer.window_set_vsync_mode(_previous_vsync)


func _process(delta: float) -> void:
	if not _running:
		return
	_elapsed += delta
	if _elapsed > WARMUP:
		var ms := delta * 1000.0
		_frames.append(ms)
		if ms > _worst_ms:
			_worst_ms = ms
			_worst_at = _elapsed
	# Ráfagas de cambios de otras fuentes, compitiendo con el laberinto vivo.
	_burst_timer -= delta
	if _burst_timer <= 0.0:
		_burst_timer = 1.0
		_maze.request_random_shift(2, Maze.PRIORITY_PLAYER, SOURCE_PLAYER)
	_director_timer -= delta
	if _director_timer <= 0.0:
		_director_timer = 2.0
		_maze.request_random_shift(3, Maze.PRIORITY_DIRECTOR, SOURCE_DIRECTOR)
	if _elapsed >= _config.stress_seconds + WARMUP:
		_finish()


func _on_window(snapshot: Dictionary) -> void:
	if not _running or _elapsed <= WARMUP:
		return
	for key: String in snapshot:
		var s: Dictionary = snapshot[key]
		var acc: Dictionary = _systems.get(key, {"sum_ms": 0.0, "max_ms": 0.0, "windows": 0})
		acc.sum_ms += s.total_ms
		acc.max_ms = maxf(acc.max_ms, s.max_ms)
		acc.windows += 1
		_systems[key] = acc


func _spawn_wanderers(count: int) -> void:
	var size := Vector2(_maze.get_grid_size()) * _maze.get_tile_px()
	var rng := RandomNumberGenerator.new()
	rng.seed = _maze.get_last_seed()
	for i in count:
		var p: Participant = PARTICIPANT_SCENE.instantiate()
		p.get_node("Vision").free()  # solo el jugador tiene luz de visión
		p.display_name = "Estrés %d" % i
		p.color = Color.from_hsv(rng.randf(), 0.55, 0.95)
		p.participant_id = 100 + i
		_match.add_child(p)
		p.global_position = _maze.nearest_floor_world(Vector2(rng.randf() * size.x, rng.randf() * size.y))
		p.set_controller(WanderController.new())


func _finish() -> void:
	_running = false
	var sorted := _frames.duplicate()
	sorted.sort()
	var n := sorted.size()
	var avg := 0.0
	for ms in sorted:
		avg += ms
	avg /= maxf(n, 1)
	var p95: float = sorted[int(n * 0.95)] if n > 0 else 0.0
	var p99: float = sorted[int(n * 0.99)] if n > 0 else 0.0
	var worst: float = sorted[n - 1] if n > 0 else 0.0
	var over_good := 0
	var over_bad := 0
	for ms in sorted:
		over_good += 1 if ms > GOOD_MS else 0
		over_bad += 1 if ms > BAD_MS else 0
	var ok := p99 <= GOOD_MS and worst <= BAD_MS
	# Sistemas más caros (promedio de ms por segundo).
	var keys := _systems.keys()
	keys.sort_custom(func(a: String, b: String) -> bool:
		return _systems[a].sum_ms / _systems[a].windows > _systems[b].sum_ms / _systems[b].windows)
	var top: PackedStringArray = []
	for i in mini(4, keys.size()):
		var s: Dictionary = _systems[keys[i]]
		top.append("%s %.2f ms/s (peor %.2f)" % [keys[i], s.sum_ms / s.windows, s.max_ms])
	var details := "%d entidades · mapa %d×%d · %d frames en %d s (sin los %d s de calentamiento)\n" % [
		_config.stress_entities, _maze.get_grid_size().x, _maze.get_grid_size().y, n, int(_elapsed - WARMUP), int(WARMUP)]
	details += "promedio %.2f ms (%d FPS) · p95 %.2f · p99 %.2f · peor %.2f ms (en el segundo %.1f)\n" % [
		avg, roundi(1000.0 / maxf(avg, 0.001)), p95, p99, worst, _worst_at]
	details += "frames > 16,7 ms: %d · > 33 ms: %d · órdenes descartadas: %d\n" % [over_good, over_bad, _maze.get_dropped_changes()]
	details += "Más caros: " + " · ".join(top)
	print("[estrés] ", details.replace("\n", " | "))
	var results := RESULTS_SCREEN.instantiate()
	_match.add_child(results)
	results.show_result("DENTRO DEL PRESUPUESTO" if ok else "FUERA DEL PRESUPUESTO",
		Color(0.45, 1.0, 0.6) if ok else Color(1.0, 0.45, 0.4), details)
	App.show_results()
