extends CanvasLayer
## Flujo del juego (autoload "App"): qué pantalla está activa y cómo se pasa de una a otra.
##
## Patrón State: cada estado es una pantalla o situación con transiciones permitidas.
##
##   MAIN_MENU ──► SETUP ──► PLAYING ⇄ PAUSED
##       ▲           │          │
##       └───────────┴── RESULTS ┘   (desde cualquier estado se puede volver al menú)
##
## Las pantallas no se cambian entre sí directamente: piden a App la transición (un solo lugar
## decide el flujo, con fundido de pantalla). Ver docs/ARQUITECTURA.md §13.

signal state_changed(previous: State, current: State)

enum State { BOOT, MAIN_MENU, SETUP, PLAYING, PAUSED, RESULTS }

const SCENES := {
	State.MAIN_MENU: "res://scenes/menus/main_menu.tscn",
	State.SETUP: "res://scenes/menus/match_setup.tscn",
	State.PLAYING: "res://scenes/game/match.tscn",
}
const FADE_TIME := 0.18

var state: State = State.BOOT
var match_config: MatchConfig = MatchConfig.create_default()

var _fade: ColorRect
var _busy := false


func _ready() -> void:
	layer = 100
	process_mode = Node.PROCESS_MODE_ALWAYS
	_fade = ColorRect.new()
	_fade.color = Color(0.01, 0.01, 0.02, 1.0)
	_fade.set_anchors_preset(Control.PRESET_FULL_RECT)
	_fade.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_fade.modulate.a = 0.0
	add_child(_fade)
	# La escena con la que arranca el proyecto define el estado inicial.
	var current := get_tree().current_scene
	if current != null:
		for s in SCENES:
			if SCENES[s] == current.scene_file_path:
				state = s


func go_to_main_menu() -> void:
	_change_scene(State.MAIN_MENU)


func go_to_setup() -> void:
	_change_scene(State.SETUP)


## Empieza una partida con la configuración dada (o la última usada).
func start_match(config: MatchConfig = null) -> void:
	if config != null:
		match_config = config
	_change_scene(State.PLAYING)


## Reinicia la partida: misma configuración (si la semilla es 0, sale otro mapa).
func restart_match() -> void:
	_change_scene(State.PLAYING)


func pause() -> void:
	if state == State.PLAYING:
		get_tree().paused = true
		_set_state(State.PAUSED)


func resume() -> void:
	if state == State.PAUSED:
		get_tree().paused = false
		_set_state(State.PLAYING)


func show_results() -> void:
	if state == State.PLAYING:
		_set_state(State.RESULTS)


func quit_game() -> void:
	get_tree().quit()


func _set_state(next: State) -> void:
	var previous := state
	state = next
	state_changed.emit(previous, next)


func _change_scene(next: State) -> void:
	if _busy:
		return
	_busy = true
	get_tree().paused = false
	var tween := create_tween()
	tween.tween_property(_fade, "modulate:a", 1.0, FADE_TIME)
	await tween.finished
	get_tree().change_scene_to_file(SCENES[next])
	_set_state(next)
	await get_tree().process_frame
	var back := create_tween()
	back.tween_property(_fade, "modulate:a", 0.0, FADE_TIME)
	await back.finished
	_busy = false
