class_name HUD
extends CanvasLayer
## HUD del jugador: barra superior delgada (stats, efectos activos, contador), pista F1 y panel
## de controles. El centro de la pantalla queda siempre libre.
##
## Observer: escucha al StatsComponent del participante y a Events; nunca modifica el juego.
## Ver docs/ARQUITECTURA.md §10.

const COUNT_TIME := 0.35  # duración de la animación de números
const FLASH_UP := Color(0.45, 1.0, 0.6)
const FLASH_DOWN := Color(1.0, 0.45, 0.45)
const TEXT_COLOR := Color(0.93, 0.92, 0.98)
const HINT_SECONDS := 5.0

## Texto del panel de controles (cada escena puede poner el suyo).
@export_multiline var controls_text := "WASD / flechas · moverse\nF1 · mostrar u ocultar esta ayuda"

@onready var _dot: ColorRect = $TopBar/Row/Dot
@onready var _speed: Label = $TopBar/Row/Speed
@onready var _vision: Label = $TopBar/Row/Vision
@onready var _effects: HBoxContainer = $TopBar/Row/Effects
@onready var _collected: Label = $TopBar/Row/Collected
@onready var _hint: Label = $HintF1
@onready var _controls: PanelContainer = $Controls
@onready var _controls_list: Label = $Controls/Rows/List
@onready var _trophy_status: Label = $TopBar/Row/TrophyStatus
@onready var _objective: ObjectiveIndicator = $Objective
@onready var _banner: PanelContainer = $Banner
@onready var _banner_title: Label = $Banner/Rows/Title
@onready var _banner_subtitle: Label = $Banner/Rows/Subtitle
@onready var _map_info: Label = $MapInfo
@onready var _fps: Label = $TopBar/Row/Fps
@onready var _toast: Label = $Toast
var _toast_tween: Tween

var _participant: Participant
var _stats: StatsComponent
var _shown := {StatsComponent.STAT_SPEED: 0.0, StatsComponent.STAT_VISION: 0.0}
var _count_tweens := {}
var _chips := {}  # id de modificador -> {panel, bar, duration}
var _collected_count := 0
var _trophy: TrophyController
var _maze: Maze

const TROPHY_COLOR := Color(1.0, 0.82, 0.25)


func _ready() -> void:
	_controls_list.text = controls_text
	Events.pickup_collected.connect(_on_pickup_collected)
	Events.trophy_taken.connect(_on_trophy_changed.unbind(1))
	Events.trophy_dropped.connect(_on_trophy_changed.unbind(2))
	Events.trophy_delivered.connect(_on_trophy_changed.unbind(1))
	Events.maze_shift_warned.connect(func(_c: Array, _o: Array, seconds: float) -> void:
		toast("¡EL LABERINTO SE MUEVE!", Color(1.0, 0.4, 0.35), seconds))
	var hint_tween := create_tween()
	hint_tween.tween_interval(HINT_SECONDS)
	hint_tween.tween_property(_hint, "modulate:a", 0.0, 1.0)


## Conecta el HUD a un participante (el que controla esta pantalla).
func bind(participant: Participant) -> void:
	_participant = participant
	_stats = participant.get_stats()
	_dot.color = participant.color
	if _stats == null:
		return
	_stats.stat_changed.connect(_on_stat_changed)
	for stat in _shown:
		_shown[stat] = _stats.get_stat(stat)
		_render_stat(stat, _shown[stat])


## Conecta el indicador de objetivo al trofeo (y al laberinto, para medir distancias en casillas).
func bind_objective(trophy: TrophyController, maze: Maze) -> void:
	_trophy = trophy
	_maze = maze
	_on_trophy_changed()


## Cartel centrado (fin de partida). Es lo único que ocupa el centro, y solo al terminar.
func show_banner(title: String, subtitle: String, color: Color) -> void:
	_banner_title.text = title
	_banner_title.add_theme_color_override("font_color", color)
	_banner_subtitle.text = subtitle
	_banner.visible = true
	_banner.pivot_offset = _banner.size / 2.0
	_banner.scale = Vector2(0.7, 0.7)
	_banner.modulate.a = 0.0
	var t := create_tween().set_parallel(true)
	t.tween_property(_banner, "modulate:a", 1.0, 0.25)
	t.tween_property(_banner, "scale", Vector2.ONE, 0.45).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)


## Información del mapa (abajo a la izquierda, discreta). Destella al cambiar.
func set_map_info(text: String) -> void:
	_map_info.text = text
	_map_info.modulate = Color(1.6, 1.5, 1.0)
	create_tween().tween_property(_map_info, "modulate", Color.WHITE, 0.8)


## Aviso breve arriba al centro (debajo de la barra): entra, late y se va. No tapa el centro.
func toast(text: String, color: Color, seconds := 2.0) -> void:
	_toast.text = text
	_toast.add_theme_color_override("font_color", color)
	_toast.visible = true
	_toast.pivot_offset = _toast.size / 2.0
	if _toast_tween and _toast_tween.is_valid():
		_toast_tween.kill()
	_toast.modulate.a = 0.0
	_toast.scale = Vector2(1.25, 1.25)
	_toast_tween = create_tween()
	_toast_tween.set_parallel(true)
	_toast_tween.tween_property(_toast, "modulate:a", 1.0, 0.15)
	_toast_tween.tween_property(_toast, "scale", Vector2.ONE, 0.25).set_trans(Tween.TRANS_BACK)
	_toast_tween.chain().tween_interval(maxf(seconds - 0.6, 0.3))
	_toast_tween.chain().tween_property(_toast, "modulate:a", 0.0, 0.4)


func hide_banner() -> void:
	_banner.visible = false


func reset_collected() -> void:
	_collected_count = 0
	_collected.text = "OBJ 0"


func set_controls_text(text: String) -> void:
	controls_text = text
	if is_node_ready():
		_controls_list.text = text


func is_controls_open() -> bool:
	return _controls.visible


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo and event.keycode == KEY_F1:
		_toggle_controls()
		get_viewport().set_input_as_handled()


func _toggle_controls() -> void:
	var opening := not _controls.visible
	_hint.modulate.a = 0.0
	if opening:
		_controls.visible = true
		_controls.modulate.a = 0.0
		_controls.scale = Vector2(0.96, 0.96)
		_controls.pivot_offset = _controls.size / 2.0
		var t := create_tween().set_parallel(true)
		t.tween_property(_controls, "modulate:a", 1.0, 0.15)
		t.tween_property(_controls, "scale", Vector2.ONE, 0.15).set_trans(Tween.TRANS_QUAD)
	else:
		var t := create_tween()
		t.tween_property(_controls, "modulate:a", 0.0, 0.12)
		t.tween_callback(func() -> void: _controls.visible = false)


func _process(_delta: float) -> void:
	if _stats != null:
		_sync_effect_chips()
	_update_objective()
	_fps.visible = Settings.get_value("show_fps")
	if _fps.visible:
		_fps.text = "%d FPS" % Engine.get_frames_per_second()


# --- Objetivo: trofeo o base -------------------------------------------------------------

func _update_objective() -> void:
	if _trophy == null or _participant == null or _trophy.is_finished():
		_objective.active = false
		return
	var carrying := _trophy.get_carrier() == _participant
	_objective.active = true
	if carrying:
		_objective.target_position = _trophy.get_base_position(_participant)
		_objective.target_name = "TU BASE"
		_objective.target_color = _participant.color
	else:
		_objective.target_position = _trophy.get_trophy_position()
		_objective.target_name = "TROFEO"
		_objective.target_color = TROPHY_COLOR
	var tile := _maze.get_tile_px() if _maze != null else 16.0
	_objective.distance_tiles = _participant.global_position.distance_to(_objective.target_position) / tile


func _on_trophy_changed() -> void:
	if _trophy == null or _participant == null:
		return
	var carrying := _trophy.get_carrier() == _participant
	if carrying != _trophy_status.visible:
		_trophy_status.visible = carrying
		if carrying:
			_trophy_status.modulate.a = 0.0
			create_tween().tween_property(_trophy_status, "modulate:a", 1.0, 0.25)


# --- Stats: números que cuentan y destellan ------------------------------------------------

func _on_stat_changed(stat: int, value: float) -> void:
	if not _shown.has(stat):
		return
	var label := _label_for(stat)
	var from: float = _shown[stat]
	if is_equal_approx(from, value):
		return
	if _count_tweens.has(stat):
		_count_tweens[stat].kill()
	var tween := create_tween().set_parallel(true)
	tween.tween_method(_set_shown.bind(stat), from, value, COUNT_TIME)
	label.add_theme_color_override("font_color", FLASH_UP if value > from else FLASH_DOWN)
	label.pivot_offset = label.size / 2.0
	label.scale = Vector2(1.15, 1.15)
	tween.tween_property(label, "scale", Vector2.ONE, COUNT_TIME).set_trans(Tween.TRANS_BACK)
	tween.chain().tween_callback(func() -> void: label.add_theme_color_override("font_color", TEXT_COLOR))
	_count_tweens[stat] = tween


func _set_shown(value: float, stat: int) -> void:
	_shown[stat] = value
	_render_stat(stat, value)


func _render_stat(stat: int, value: float) -> void:
	match stat:
		StatsComponent.STAT_SPEED:
			_speed.text = "VEL %d" % roundi(value)
		StatsComponent.STAT_VISION:
			_vision.text = "VIS %.1f" % value


func _label_for(stat: int) -> Label:
	return _speed if stat == StatsComponent.STAT_SPEED else _vision


# --- Efectos temporales como fichas con barra que se vacía --------------------------------

func _sync_effect_chips() -> void:
	var alive := {}
	for m: Dictionary in _stats.get_modifiers():
		if m.duration < 0.0:
			continue  # los permanentes ya se ven en el valor de la stat
		alive[m.id] = true
		if not _chips.has(m.id):
			_chips[m.id] = _make_chip(m)
		var chip: Dictionary = _chips[m.id]
		chip.bar.value = clampf(m.remaining / chip.duration, 0.0, 1.0)
	for id in _chips.keys():
		if not alive.has(id):
			_remove_chip(_chips[id])
			_chips.erase(id)


func _make_chip(m: Dictionary) -> Dictionary:
	var color: Color = EffectText.STAT_COLORS.get(m.stat, Color.WHITE)
	var panel := PanelContainer.new()
	var style := StyleBoxFlat.new()
	style.bg_color = Color(color, 0.16)
	style.border_color = Color(color, 0.7)
	style.set_border_width_all(1)
	style.set_corner_radius_all(4)
	style.content_margin_left = 8
	style.content_margin_right = 8
	style.content_margin_top = 1
	style.content_margin_bottom = 2
	panel.add_theme_stylebox_override("panel", style)
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 1)
	var label := Label.new()
	label.text = EffectText.format(m.stat, m.op, m.value)
	label.add_theme_font_size_override("font_size", 14)
	label.add_theme_color_override("font_color", color)
	var bar := ProgressBar.new()
	bar.show_percentage = false
	bar.max_value = 1.0
	bar.custom_minimum_size = Vector2(0, 3)
	var fill := StyleBoxFlat.new()
	fill.bg_color = color
	var bg := StyleBoxFlat.new()
	bg.bg_color = Color(color, 0.2)
	bar.add_theme_stylebox_override("fill", fill)
	bar.add_theme_stylebox_override("background", bg)
	box.add_child(label)
	box.add_child(bar)
	panel.add_child(box)
	_effects.add_child(panel)
	# Entra con un destello (el contenedor controla la posición; se anima opacidad y escala).
	panel.modulate.a = 0.0
	panel.scale = Vector2(1.25, 1.25)
	var t := create_tween().set_parallel(true)
	t.tween_property(panel, "modulate:a", 1.0, 0.2)
	t.tween_property(panel, "scale", Vector2.ONE, 0.25).set_trans(Tween.TRANS_BACK)
	return {"panel": panel, "bar": bar, "duration": maxf(m.duration, 0.001)}


func _remove_chip(chip: Dictionary) -> void:
	var panel: Control = chip.panel
	var t := create_tween()
	t.tween_property(panel, "modulate:a", 0.0, 0.25)
	t.tween_callback(panel.queue_free)


# --- Contador -------------------------------------------------------------------------------

func _on_pickup_collected(_type: PickupType, participant: Participant, _pos: Vector2) -> void:
	if participant != _participant:
		return
	_collected_count += 1
	_collected.text = "OBJ %d" % _collected_count
	_collected.pivot_offset = _collected.size / 2.0
	_collected.scale = Vector2(1.2, 1.2)
	create_tween().tween_property(_collected, "scale", Vector2.ONE, 0.2).set_trans(Tween.TRANS_BACK)
