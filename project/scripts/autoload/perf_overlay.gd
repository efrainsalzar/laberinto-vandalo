extends CanvasLayer
## Panel de rendimiento (autoload "Perf"). F3 lo muestra u oculta en cualquier pantalla.
##
## Muestra en vivo:
## - FPS, tiempo de frame promedio y el PEOR frame del último segundo (detector de tirones).
## - Gráfico de los últimos frames con líneas de 16,7 ms (60 FPS) y 33 ms (30 FPS).
## - Tiempos de Godot (proceso, física), llamadas de dibujo y nodos.
## - Tiempos de NUESTROS sistemas (C++, medidos con lv::core::PerfScope): total por segundo, peor llamada
##   y cantidad de llamadas. Cada sistema nuevo agrega su medición y aparece aquí solo.
## Además publica todo como monitores del depurador de Godot (pestaña Depurador › Monitores).
## Ver docs/RENDIMIENTO.md.

## Cada segundo: {sistema: {calls, total_ms, max_ms, last_ms}} (lo usa la prueba de estrés).
signal window_closed(snapshot: Dictionary)

const HISTORY := 180  # frames en el gráfico
const GOOD_MS := 1000.0 / 60.0
const BAD_MS := 1000.0 / 30.0
const BG := Color(0.03, 0.02, 0.06, 0.88)

var _panel: PanelContainer
var _text: RichTextLabel
var _graph: Control
var _frames: PackedFloat32Array = []
var _window_time := 0.0
var _window_worst := 0.0
var _window_sum := 0.0
var _window_count := 0
var _last_worst := 0.0
var _last_avg := 0.0
var _snapshot := {}
var _monitors := {}


func _ready() -> void:
	layer = 95
	process_mode = Node.PROCESS_MODE_ALWAYS
	_build()
	_panel.visible = Settings.get_value("show_perf")
	Settings.changed.connect(func(key: String, value: Variant) -> void:
		if key == "show_perf":
			_panel.visible = value)
	Performance.add_custom_monitor("Laberinto/peor_frame_ms", func() -> float: return _last_worst)


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo and event.keycode == KEY_F3:
		Settings.set_value("show_perf", not _panel.visible)
		get_viewport().set_input_as_handled()


func _process(delta: float) -> void:
	var ms := delta * 1000.0
	_frames.append(ms)
	if _frames.size() > HISTORY:
		_frames.remove_at(0)
	_window_worst = maxf(_window_worst, ms)
	_window_sum += ms
	_window_count += 1
	_window_time += delta
	if _window_time >= 1.0:
		_close_window()
	if _panel.visible:
		_graph.queue_redraw()


func _close_window() -> void:
	_last_worst = _window_worst
	_last_avg = _window_sum / maxf(_window_count, 1)
	_window_time = 0.0
	_window_worst = 0.0
	_window_sum = 0.0
	_window_count = 0
	PerfStats.roll_window()
	_snapshot = PerfStats.snapshot()
	window_closed.emit(_snapshot)
	_register_monitors()
	if _panel.visible:
		_update_text()


func _register_monitors() -> void:
	for key: String in _snapshot:
		if _monitors.has(key):
			continue
		_monitors[key] = true
		var id := "Laberinto/%s_max_ms" % key.replace(".", "_")
		if not Performance.has_custom_monitor(id):
			Performance.add_custom_monitor(id, func() -> float: return _snapshot.get(key, {}).get("max_ms", 0.0))


func _color_for(ms: float) -> String:
	if ms > BAD_MS:
		return "#ff6b5e"
	if ms > GOOD_MS:
		return "#ffd166"
	return "#7ee08a"


func _update_text() -> void:
	var lines: PackedStringArray = []
	lines.append("[b]RENDIMIENTO[/b]  [color=#8a84a8](F3)[/color]")
	lines.append("FPS [b]%d[/b] · frame %.1f ms · peor [color=%s][b]%.1f ms[/b][/color]" % [
		Engine.get_frames_per_second(), _last_avg, _color_for(_last_worst), _last_worst])
	lines.append("[color=#8a84a8]Godot: proceso %.2f ms · física %.2f ms · dibujos %d · nodos %d[/color]" % [
		Performance.get_monitor(Performance.TIME_PROCESS) * 1000.0,
		Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS) * 1000.0,
		Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
		Performance.get_monitor(Performance.OBJECT_NODE_COUNT)])
	if _snapshot.is_empty():
		lines.append("[color=#8a84a8]sistemas: sin mediciones todavía[/color]")
	else:
		lines.append("[color=#ffd166]sistema            ms/s    peor    llamadas[/color]")
		var keys := _snapshot.keys()
		keys.sort()
		for key: String in keys:
			var s: Dictionary = _snapshot[key]
			if s.calls == 0:
				continue
			lines.append("%-18s %6.2f  [color=%s]%6.2f[/color]  %5d" % [key, s.total_ms, _color_for(s.max_ms), s.max_ms, s.calls])
	_text.text = "\n".join(lines)


func _build() -> void:
	_panel = PanelContainer.new()
	var style := StyleBoxFlat.new()
	style.bg_color = BG
	style.border_color = Color(0.45, 0.4, 0.65, 0.6)
	style.set_border_width_all(1)
	style.set_corner_radius_all(6)
	style.content_margin_left = 10
	style.content_margin_right = 10
	style.content_margin_top = 8
	style.content_margin_bottom = 8
	_panel.add_theme_stylebox_override("panel", style)
	_panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_panel.set_anchors_preset(Control.PRESET_TOP_RIGHT)
	_panel.grow_horizontal = Control.GROW_DIRECTION_BEGIN
	_panel.offset_left = -16
	_panel.offset_right = -16
	_panel.offset_top = 46
	var box := VBoxContainer.new()
	_text = RichTextLabel.new()
	_text.bbcode_enabled = true
	_text.fit_content = true
	_text.scroll_active = false
	_text.custom_minimum_size = Vector2(390, 0)
	_text.add_theme_font_size_override("normal_font_size", 13)
	_text.add_theme_font_size_override("bold_font_size", 13)
	_text.add_theme_font_override("normal_font", _mono_font())
	_text.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_graph = Control.new()
	_graph.custom_minimum_size = Vector2(390, 56)
	_graph.draw.connect(_draw_graph)
	_graph.mouse_filter = Control.MOUSE_FILTER_IGNORE
	box.add_child(_text)
	box.add_child(_graph)
	_panel.add_child(box)
	add_child(_panel)
	_update_text()


func _mono_font() -> Font:
	var f := SystemFont.new()
	f.font_names = PackedStringArray(["Consolas", "DejaVu Sans Mono", "Menlo", "monospace"])
	return f


func _draw_graph() -> void:
	var size := _graph.size
	_graph.draw_rect(Rect2(Vector2.ZERO, size), Color(0, 0, 0, 0.35))
	var scale_ms := 50.0  # alto del gráfico = 50 ms
	for limit: float in [GOOD_MS, BAD_MS]:
		var y := size.y - size.y * limit / scale_ms
		_graph.draw_line(Vector2(0, y), Vector2(size.x, y), Color(_color_for(limit + 0.01), 0.5), 1.0)
	if _frames.size() < 2:
		return
	var step := size.x / float(HISTORY - 1)
	for i in _frames.size():
		var ms := _frames[i]
		var h := minf(ms / scale_ms, 1.0) * size.y
		var x := i * step
		_graph.draw_line(Vector2(x, size.y), Vector2(x, size.y - h), Color(_color_for(ms)), maxf(step, 1.0))
