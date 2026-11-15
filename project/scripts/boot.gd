extends Control
## Escena de arranque provisional (v0.0.x).
## Solo verifica que la extensión C++ esté cargada. Se reemplazará por el menú / la partida.

@onready var _estado: Label = $Centro/Estado


func _ready() -> void:
	var version: String = ProjectSettings.get_setting("application/config/version")
	var cpp_ok := GDExtensionManager.is_extension_loaded("res://bin/laberinto.gdextension")
	if cpp_ok:
		_estado.text = "v%s · base lista · extensión C++ cargada" % version
	else:
		_estado.text = "v%s · ERROR: la extensión C++ no cargó (¿compilaste con scons?)" % version
		push_error("La extensión C++ no está cargada.")
	print("[boot] cpp_ok=", cpp_ok)
