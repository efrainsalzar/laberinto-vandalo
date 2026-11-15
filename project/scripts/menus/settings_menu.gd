extends Control
## Panel de opciones. Se usa desde el menú principal y desde la pausa (como capa encima).
## Solo lee y escribe en Settings; Settings guarda y aplica. Emite `closed` al volver.

signal closed

@onready var _fullscreen: CheckButton = %Fullscreen
@onready var _vsync: CheckButton = %VSync
@onready var _show_fps: CheckButton = %ShowFps
@onready var _debug_keys: CheckButton = %DebugKeys
@onready var _zoom: HSlider = %Zoom
@onready var _zoom_label: Label = %ZoomLabel


func _ready() -> void:
	_fullscreen.button_pressed = Settings.get_value("fullscreen")
	_vsync.button_pressed = Settings.get_value("vsync")
	_show_fps.button_pressed = Settings.get_value("show_fps")
	_debug_keys.button_pressed = Settings.get_value("debug_keys")
	_zoom.value = Settings.get_value("camera_zoom")
	_update_zoom_label(_zoom.value)
	_fullscreen.toggled.connect(func(on: bool) -> void: Settings.set_value("fullscreen", on))
	_vsync.toggled.connect(func(on: bool) -> void: Settings.set_value("vsync", on))
	_show_fps.toggled.connect(func(on: bool) -> void: Settings.set_value("show_fps", on))
	_debug_keys.toggled.connect(func(on: bool) -> void: Settings.set_value("debug_keys", on))
	_zoom.value_changed.connect(func(v: float) -> void:
		Settings.set_value("camera_zoom", v)
		_update_zoom_label(v))
	%Back.pressed.connect(close)
	UiFx.hint(_fullscreen, "Ocupa toda la pantalla.")
	UiFx.hint(_vsync, "Sincroniza los cuadros con el monitor: evita cortes en la imagen. Apágalo para medir el rendimiento real.")
	UiFx.hint(_show_fps, "Muestra los cuadros por segundo en la barra superior durante la partida.")
	UiFx.hint(_debug_keys, "Activa teclas para probar cosas en la partida (efectos, soltar el trofeo, quitar la niebla, ver el mapa). Ver F1.")
	UiFx.hint(_zoom, "Qué tan cerca está la cámara del jugador. Menos zoom = ves más mapa.")
	_fullscreen.grab_focus.call_deferred()


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_cancel"):
		close()
		get_viewport().set_input_as_handled()


func close() -> void:
	closed.emit()
	queue_free()


func _update_zoom_label(v: float) -> void:
	_zoom_label.text = "Zoom de la cámara: %.2f" % v
