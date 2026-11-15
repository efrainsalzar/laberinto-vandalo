extends CanvasLayer
## Menú de pausa (capa encima de la partida). Funciona aunque el árbol esté en pausa.
## La partida lo crea al pulsar Esc; se destruye al continuar. Solo pide acciones a App.

signal resumed

var _settings: Control


func _ready() -> void:
	%Resume.pressed.connect(resume)
	%Restart.pressed.connect(App.restart_match)
	%NewMap.pressed.connect(func() -> void:
		App.match_config.seed = 0
		App.restart_match())
	%Options.pressed.connect(_open_settings)
	%Menu.pressed.connect(App.go_to_main_menu)
	%Quit.pressed.connect(App.quit_game)
	UiFx.hint(%Restart, "Empieza de nuevo en este mismo mapa.")
	UiFx.hint(%NewMap, "Empieza de nuevo con otro mapa de la misma dificultad y tamaño.")
	UiFx.hint(%Menu, "Abandona la partida y vuelve al menú principal.")
	var panel: Control = %Panel
	panel.pivot_offset = panel.size / 2.0
	panel.scale = Vector2(0.92, 0.92)
	panel.modulate.a = 0.0
	var t := create_tween().set_parallel(true)
	t.tween_property(panel, "scale", Vector2.ONE, 0.18).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	t.tween_property(panel, "modulate:a", 1.0, 0.12)
	%Resume.grab_focus.call_deferred()


func _unhandled_input(event: InputEvent) -> void:
	if _settings == null and event.is_action_pressed("pause"):
		resume()
		get_viewport().set_input_as_handled()


func resume() -> void:
	App.resume()
	resumed.emit()
	queue_free()


func _open_settings() -> void:
	%Panel.visible = false
	_settings = load("res://scenes/menus/settings_menu.tscn").instantiate()
	add_child(_settings)
	_settings.closed.connect(func() -> void:
		_settings = null
		%Panel.visible = true
		%Options.grab_focus())
