class_name HUD
extends CanvasLayer
## HUD del jugador, todo flotante (sin barras): tarjeta del participante abajo a la izquierda (estado, stats,
## efectos y armas), pastilla del reloj arriba al centro con la línea del trofeo, FPS opcional arriba a la derecha,
## pista F1 y panel de controles. El centro de la pantalla queda libre.
##
## Observer: escucha al participante y a Events; nunca modifica el juego.
## Ver docs/ARQUITECTURA.md §10 y docs/arquitectura/24-tarjeta-participante.md.

const HINT_SECONDS := 5.0

## Texto del panel de controles (cada escena puede poner el suyo).
@export_multiline var controls_text := "WASD / flechas · moverse\nF1 · mostrar u ocultar esta ayuda"

@onready var _hint: Label = $HintF1
@onready var _controls: PanelContainer = $Controls
@onready var _controls_list: Label = $Controls/Rows/List
@onready var _objective: ObjectiveIndicator = $Objective
@onready var _banner: PanelContainer = $Banner
@onready var _banner_title: Label = $Banner/Rows/Title
@onready var _banner_subtitle: Label = $Banner/Rows/Subtitle
@onready var _map_info: Label = $MapInfo
@onready var _fps: Label = $Fps
@onready var _toast: Label = $Toast
@onready var _clock: Control = $Clock
@onready var _clock_pill: PanelContainer = $Clock/Pill
@onready var _clock_time: Label = $Clock/Pill/Row/Time
@onready var _clock_phase: Label = $Clock/Pill/Row/Phase
@onready var _trophy_line: Label = $Clock/Trophy
var _map_text := ""
var _clock_warn := false
const PLAYER_CARD := preload("res://scripts/ui/player_card/player_card.gd")
var _card: Control
var _toast_tween: Tween

var _participant: Participant
var _collected_count := 0
var _trophy: TrophyController
var _maze: Maze
var _match: MatchController
var _clock_shown := -2  # segundo mostrado: el reloj solo se redibuja cuando cambia

const CLOCK_WARN := Color(1.0, 0.4, 0.35)
const PHASE_TOASTS := {
	MatchController.PHASE_PREPARATION: ["FASE 1 · PREPARACIÓN", "Explora y junta mejoras", Color(0.55, 0.85, 1.0)],
	MatchController.PHASE_ASSAULT: ["FASE 2 · ASALTO", "¡Ve por el trofeo!", Color(1.0, 0.82, 0.25)],
	MatchController.PHASE_HEIST: ["FASE 3 · GOLPE DE LOS VÁNDALOS", "El laberinto se vuelve loco", Color(1.0, 0.4, 0.35)],
}

const TROPHY_COLOR := Color(1.0, 0.82, 0.25)


func _ready() -> void:
	_controls_list.text = controls_text
	Events.pickup_collected.connect(_on_pickup_collected)
	# Línea del trofeo debajo del reloj: dónde está o quién lo lleva.
	Events.trophy_taken.connect(func(p: Participant) -> void: _set_trophy_line(p, "taken"))
	Events.trophy_stolen.connect(func(thief: Participant, _v: Participant) -> void: _set_trophy_line(thief, "taken"))
	Events.trophy_dropped.connect(func(_p: Participant, _pos: Vector2) -> void: _set_trophy_line(null, "floor"))
	Events.trophy_delivered.connect(func(p: Participant) -> void: _set_trophy_line(p, "delivered"))
	Events.match_phase_changed.connect(_on_phase_changed)
	Events.match_time_warning.connect(func(seconds_left: float) -> void:
		var text := "¡QUEDA 1 MINUTO!" if seconds_left >= 60.0 else "¡QUEDAN %d SEGUNDOS!" % roundi(seconds_left)
		if seconds_left > 60.0:
			text = "¡QUEDAN %d MINUTOS!" % roundi(seconds_left / 60.0)
		toast(text, CLOCK_WARN, 2.5))
	Events.maze_shift_warned.connect(func(_c: Array, _o: Array, seconds: float) -> void:
		toast("¡EL LABERINTO SE MUEVE!", Color(1.0, 0.4, 0.35), seconds))
	var hint_tween := create_tween()
	hint_tween.tween_interval(HINT_SECONDS)
	hint_tween.tween_property(_hint, "modulate:a", 0.0, 1.0)


## Conecta el HUD a un participante (el que controla esta pantalla).
func bind(participant: Participant) -> void:
	_participant = participant
	if _card == null:
		_card = PLAYER_CARD.new()
		add_child(_card)
		move_child(_card, 0)  # debajo de los carteles y del panel de controles
	_card.bind(participant)


## Conecta el indicador de objetivo al trofeo (y al laberinto, para medir distancias en casillas).
func bind_objective(trophy: TrophyController, maze: Maze) -> void:
	_trophy = trophy
	_maze = maze
	_set_trophy_line(null, "vault")
	if _card != null:
		_card.set_trophy(trophy)


## Conecta el reloj de arriba al centro a la partida.
func bind_match(match_controller: MatchController) -> void:
	_match = match_controller
	_clock_shown = -2
	_clock.visible = true


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


## Información del mapa: ya no ocupa la pantalla; se muestra al pie de la ayuda (F1) y en los resultados.
func set_map_info(text: String) -> void:
	_map_text = text
	_map_info.text = text
	_refresh_controls_text()


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


## Objetos recogidos en la partida (para la pantalla de resultados).
func get_collected_text() -> String:
	return "%d objetos" % _collected_count


func set_controls_text(text: String) -> void:
	controls_text = text
	_refresh_controls_text()


func _refresh_controls_text() -> void:
	if is_node_ready():
		_controls_list.text = controls_text + ("\n\n" + _map_text if not _map_text.is_empty() else "")


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
	_update_objective()
	_update_clock()
	_fps.visible = Settings.get_value("show_fps")
	if _fps.visible:
		_fps.text = "%d FPS" % Engine.get_frames_per_second()


# --- Reloj de la partida -------------------------------------------------------------------

func _update_clock() -> void:
	if _match == null:
		return
	var left := _match.get_time_left()
	if _match.is_unlimited():
		var elapsed := int(_match.get_elapsed())
		if elapsed != _clock_shown:
			_clock_shown = elapsed
			_clock_time.text = "%d:%02d" % [elapsed / 60, elapsed % 60]
			_clock_time.remove_theme_color_override("font_color")
		return
	var shown := ceili(left)
	if shown == _clock_shown:
		return
	_clock_shown = shown
	_clock_time.text = "%d:%02d" % [shown / 60, shown % 60]
	_set_clock_warn(left <= 60.0)
	if left <= 10.0 and _match.is_running():
		# Últimos segundos: cada segundo late.
		_clock_time.pivot_offset = _clock_time.size / 2.0
		_clock_time.scale = Vector2(1.35, 1.35)
		create_tween().tween_property(_clock_time, "scale", Vector2.ONE, 0.4).set_trans(Tween.TRANS_BACK)


## Último minuto: la pastilla se pone roja y un poco más grande.
func _set_clock_warn(warn: bool) -> void:
	if warn == _clock_warn:
		return
	_clock_warn = warn
	if warn:
		_clock_time.add_theme_color_override("font_color", CLOCK_WARN)
		var style: StyleBoxFlat = _clock_pill.get_theme_stylebox("panel").duplicate()
		style.border_color = CLOCK_WARN
		style.bg_color = Color(0.2, 0.04, 0.05, 0.85)
		_clock_pill.add_theme_stylebox_override("panel", style)
		_clock_pill.pivot_offset = _clock_pill.size / 2.0
		create_tween().tween_property(_clock_pill, "scale", Vector2(1.12, 1.12), 0.3).set_trans(Tween.TRANS_BACK)
	else:
		_clock_time.remove_theme_color_override("font_color")
		_clock_pill.scale = Vector2.ONE


## Línea del trofeo: "en la bóveda", "en el piso", "lo lleva ROJO", "¡LO LLEVAS TÚ!".
func _set_trophy_line(carrier: Participant, state: String) -> void:
	var color := TROPHY_COLOR
	var text := "♛ en la bóveda"
	match state:
		"floor":
			text = "♛ en el piso · ¡tómalo!"
		"delivered":
			text = "♛ entregado"
		"taken":
			if carrier == _participant:
				text = "♛ ¡LO LLEVAS TÚ! · a tu base"
			elif carrier:
				text = "♛ lo lleva %s" % carrier.display_name.to_upper()
				color = carrier.color
	_trophy_line.text = text
	_trophy_line.add_theme_color_override("font_color", color)
	_trophy_line.pivot_offset = _trophy_line.size / 2.0
	_trophy_line.scale = Vector2(1.15, 1.15)
	create_tween().tween_property(_trophy_line, "scale", Vector2.ONE, 0.25).set_trans(Tween.TRANS_BACK)


func _on_phase_changed(phase: int, phase_name: String) -> void:
	_clock_phase.text = phase_name.to_upper()
	if PHASE_TOASTS.has(phase):
		var info: Array = PHASE_TOASTS[phase]
		_clock_phase.add_theme_color_override("font_color", info[2])
		toast("%s — %s" % [info[0], info[1]], info[2], 3.0)


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
	elif _trophy.get_carrier() != null:
		# Lo lleva otro: la flecha apunta al portador (tócalo para quitárselo).
		_objective.target_position = _trophy.get_trophy_position()
		_objective.target_name = "QUÍTASELO"
		_objective.target_color = _trophy.get_carrier().color
	else:
		_objective.target_position = _trophy.get_trophy_position()
		_objective.target_name = "TROFEO"
		_objective.target_color = TROPHY_COLOR
	var tile := _maze.get_tile_px() if _maze != null else 16.0
	_objective.distance_tiles = _participant.global_position.distance_to(_objective.target_position) / tile


# --- Contador -------------------------------------------------------------------------------

func _on_pickup_collected(_type: PickupType, participant: Participant, _pos: Vector2) -> void:
	if participant != _participant:
		return
	_collected_count += 1
