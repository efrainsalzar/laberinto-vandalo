extends Node
## Opciones del jugador (autoload "Settings"). Se guardan en user://settings.cfg y se aplican al cambiar.
## Ver docs/ARQUITECTURA.md §13.

signal changed(key: String, value: Variant)

const PATH := "user://settings.cfg"
const SECTION := "opciones"
const DEFAULTS := {
	"fullscreen": false,
	"vsync": true,
	"show_fps": false,
	"debug_keys": false,  # teclas de prueba (1/2/3/0/T/V/Tab/M) dentro de la partida
	"camera_zoom": 2.0,
}

var _values := DEFAULTS.duplicate()


func _ready() -> void:
	var file := ConfigFile.new()
	if file.load(PATH) == OK:
		for key in DEFAULTS:
			_values[key] = file.get_value(SECTION, key, DEFAULTS[key])
	_apply_all()


func get_value(key: String) -> Variant:
	return _values.get(key, DEFAULTS.get(key))


func set_value(key: String, value: Variant) -> void:
	if _values.get(key) == value:
		return
	_values[key] = value
	_apply(key)
	_save()
	changed.emit(key, value)


func _save() -> void:
	var file := ConfigFile.new()
	for key in _values:
		file.set_value(SECTION, key, _values[key])
	file.save(PATH)


func _apply_all() -> void:
	for key in _values:
		_apply(key)


func _apply(key: String) -> void:
	if DisplayServer.get_name() == "headless":
		return
	match key:
		"fullscreen":
			DisplayServer.window_set_mode(
				DisplayServer.WINDOW_MODE_FULLSCREEN if _values.fullscreen else DisplayServer.WINDOW_MODE_WINDOWED)
		"vsync":
			DisplayServer.window_set_vsync_mode(
				DisplayServer.VSYNC_ENABLED if _values.vsync else DisplayServer.VSYNC_DISABLED)
