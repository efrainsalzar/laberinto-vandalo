extends Node
## Prueba de estrés (regla 10 de docs/RENDIMIENTO.md). La crea la partida si App.match_config.stress_test.
##
## Carga: N entidades que deambulan (WanderController), laberinto vivo Caótico en mapa Gigante, y ráfagas de
## cambios de dos fuentes falsas ("jugador" y "Director") que compiten con el laberinto vivo (prioridades y
## reservas). VSync apagado durante la prueba. Los primeros WARMUP segundos no se cuentan.
##
## Registro (para que una persona o un agente lo lea sin copiar texto de la pantalla):
## - Cada frame lento (> 33 ms) se captura EN EL MOMENTO con el tiempo de cada sistema en ese frame
##   (PerfStats.take_frame) y los tiempos de Godot: así se sabe QUIÉN causó cada tirón.
## - Al terminar: user://stress/latest.json (+ copia con fecha) y una línea en user://stress/history.csv.
## - Ejecución automática: godot --path project -- --stress [--quit-after-stress]
## Ver docs/RENDIMIENTO.md.

const PARTICIPANT_SCENE := preload("res://scenes/participant/participant.tscn")
const RESULTS_SCREEN := preload("res://scenes/menus/results_screen.tscn")
const SOURCE_PLAYER := 1001
const SOURCE_DIRECTOR := 1002
const WARMUP := 2.0
const GOOD_MS := 1000.0 / 60.0
const BAD_MS := 1000.0 / 30.0
const MAX_SPIKES := 60
const LOG_DIR := "user://stress"

var _match: Node
var _maze: Maze
var _hud: HUD
var _config: MatchConfig
var _frames: PackedFloat32Array = []
var _systems := {}  # nombre -> {sum_ms, max_ms, windows}
var _spikes: Array[Dictionary] = []
var _elapsed := 0.0
var _burst_timer := 0.0
var _director_timer := 0.0
var _running := false
var _worst_at := 0.0
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
	_spikes.clear()
	_elapsed = 0.0
	_running = true
	if DisplayServer.get_name() != "headless":
		_previous_vsync = DisplayServer.window_get_vsync_mode()
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	Perf.window_closed.connect(_on_window)
	PerfStats.take_frame()  # descartar lo acumulado durante la carga
	_hud.toast("PRUEBA DE ESTRÉS: %d s" % int(_config.stress_seconds), Color(1.0, 0.82, 0.25), 2.5)


func _exit_tree() -> void:
	if DisplayServer.get_name() != "headless":
		DisplayServer.window_set_vsync_mode(_previous_vsync)


func _process(delta: float) -> void:
	if not _running:
		return
	_elapsed += delta
	# Tiempo de cada sistema desde el frame anterior (≈ el frame que acaba de terminar).
	var frame_systems: Dictionary = PerfStats.take_frame()
	if _elapsed > WARMUP:
		var ms := delta * 1000.0
		_frames.append(ms)
		if ms > _worst_ms:
			_worst_ms = ms
			_worst_at = _elapsed
		if ms > BAD_MS and _spikes.size() < MAX_SPIKES:
			_spikes.append(_capture_spike(ms, frame_systems))
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


func _capture_spike(ms: float, frame_systems: Dictionary) -> Dictionary:
	var systems := {}
	for key: String in frame_systems:
		systems[key] = snappedf(frame_systems[key], 0.001)
	return {
		"t": snappedf(_elapsed, 0.01),
		"frame_ms": snappedf(ms, 0.01),
		"systems_ms": systems,
		"godot": {
			"process_ms": snappedf(Performance.get_monitor(Performance.TIME_PROCESS) * 1000.0, 0.01),
			"physics_ms": snappedf(Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS) * 1000.0, 0.01),
			"draw_calls": Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
			"nodes": Performance.get_monitor(Performance.OBJECT_NODE_COUNT),
			"pending_map_changes": _maze.get_pending_changes(),
		},
	}


func _on_window(snapshot: Dictionary) -> void:
	if not _running or _elapsed <= WARMUP:
		return
	for key: String in snapshot:
		var s: Dictionary = snapshot[key]
		var acc: Dictionary = _systems.get(key, {"sum_ms": 0.0, "max_ms": 0.0, "windows": 0, "calls": 0})
		acc.sum_ms += s.total_ms
		acc.max_ms = maxf(acc.max_ms, s.max_ms)
		acc.windows += 1
		acc.calls += s.calls
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
	var report := _build_report()
	var paths := _save_report(report)
	var m: Dictionary = report.frames
	var details := "%d entidades · mapa %s · %d frames en %d s (sin %d s de calentamiento)\n" % [
		report.load.entities, report.load.map_size, m.count, int(report.load.seconds), int(WARMUP)]
	details += "promedio %.2f ms (%d FPS) · p95 %.2f · p99 %.2f · peor %.2f ms (seg. %.1f)\n" % [
		m.avg_ms, m.avg_fps, m.p95_ms, m.p99_ms, m.worst_ms, m.worst_at_s]
	details += "frames > 16,7 ms: %d · > 33 ms: %d · tirones registrados: %d · órdenes descartadas: %d\n" % [
		m.over_16_ms, m.over_33_ms, report.spikes.size(), report.load.dropped_map_changes]
	var top: PackedStringArray = []
	for s: Dictionary in report.systems.slice(0, 4):
		top.append("%s %.2f ms/s (peor %.2f)" % [s.name, s.avg_ms_per_s, s.worst_ms])
	details += "Más caros: " + " · ".join(top) + "\nInforme: " + paths.latest
	print("[estrés] ", JSON.stringify(report.frames), " | veredicto: ", report.verdict, " | ", paths.latest)
	var results := RESULTS_SCREEN.instantiate()
	_match.add_child(results)
	var ok: bool = report.verdict == "DENTRO"
	results.show_result("DENTRO DEL PRESUPUESTO" if ok else "FUERA DEL PRESUPUESTO",
		Color(0.45, 1.0, 0.6) if ok else Color(1.0, 0.45, 0.4), details)
	App.show_results()
	if "--quit-after-stress" in OS.get_cmdline_user_args():
		await get_tree().create_timer(1.0).timeout
		get_tree().quit()


func _build_report() -> Dictionary:
	var sorted := _frames.duplicate()
	sorted.sort()
	var n := sorted.size()
	var total := 0.0
	var over_good := 0
	var over_bad := 0
	for ms in sorted:
		total += ms
		over_good += 1 if ms > GOOD_MS else 0
		over_bad += 1 if ms > BAD_MS else 0
	var avg := total / maxf(n, 1)
	var pct := func(p: float) -> float: return sorted[mini(int(n * p), n - 1)] if n > 0 else 0.0
	var systems: Array[Dictionary] = []
	for key: String in _systems:
		var s: Dictionary = _systems[key]
		systems.append({"name": key, "avg_ms_per_s": snappedf(s.sum_ms / s.windows, 0.001),
			"worst_ms": snappedf(s.max_ms, 0.001), "calls_per_s": snappedf(float(s.calls) / s.windows, 0.1)})
	systems.sort_custom(func(a: Dictionary, b: Dictionary) -> bool: return a.avg_ms_per_s > b.avg_ms_per_s)
	var p99: float = pct.call(0.99)
	var worst: float = sorted[n - 1] if n > 0 else 0.0
	return {
		"version": ProjectSettings.get_setting("application/config/version"),
		"date": Time.get_datetime_string_from_system(),
		"godot": Engine.get_version_info().string,
		"verdict": "DENTRO" if p99 <= GOOD_MS and worst <= BAD_MS else "FUERA",
		"budget": {"p99_max_ms": snappedf(GOOD_MS, 0.01), "worst_max_ms": snappedf(BAD_MS, 0.01)},
		"load": {
			"entities": _config.stress_entities, "seconds": snappedf(_elapsed - WARMUP, 0.1),
			"map_size": "%dx%d" % [_maze.get_grid_size().x, _maze.get_grid_size().y],
			"map_seed": _maze.get_last_seed(), "live_maze": _config.live_maze,
			"dropped_map_changes": _maze.get_dropped_changes(),
		},
		"frames": {
			"count": n, "avg_ms": snappedf(avg, 0.01), "avg_fps": roundi(1000.0 / maxf(avg, 0.001)),
			"p50_ms": snappedf(pct.call(0.5), 0.01), "p95_ms": snappedf(pct.call(0.95), 0.01),
			"p99_ms": snappedf(p99, 0.01), "worst_ms": snappedf(worst, 0.01), "worst_at_s": snappedf(_worst_at, 0.01),
			"over_16_ms": over_good, "over_33_ms": over_bad,
		},
		"systems": systems,
		"spikes": _spikes,
	}


func _save_report(report: Dictionary) -> Dictionary:
	DirAccess.make_dir_recursive_absolute(LOG_DIR)
	var text := JSON.stringify(report, "  ")
	var stamp := Time.get_datetime_string_from_system().replace(":", "-")
	for path in [LOG_DIR + "/latest.json", LOG_DIR + "/stress_%s.json" % stamp]:
		var f := FileAccess.open(path, FileAccess.WRITE)
		if f:
			f.store_string(text)
	# Historial: una línea por ejecución para comparar versiones.
	var history := LOG_DIR + "/history.csv"
	var is_new := not FileAccess.file_exists(history)
	var h := FileAccess.open(history, FileAccess.READ_WRITE if not is_new else FileAccess.WRITE)
	if h:
		h.seek_end()
		if is_new:
			h.store_line("fecha,version,veredicto,entidades,mapa,frames,avg_ms,p95_ms,p99_ms,peor_ms,peor_seg,frames_33ms,sistema_mas_caro")
		var f: Dictionary = report.frames
		var top: String = report.systems[0].name if not report.systems.is_empty() else ""
		h.store_line("%s,%s,%s,%d,%s,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%s" % [report.date, report.version, report.verdict,
			report.load.entities, report.load.map_size, f.count, f.avg_ms, f.p95_ms, f.p99_ms, f.worst_ms, f.worst_at_s,
			f.over_33_ms, top])
	return {"latest": ProjectSettings.globalize_path(LOG_DIR + "/latest.json")}
