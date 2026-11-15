extends CanvasLayer
## Pantalla de resultados al terminar una partida (capa encima del juego).

func _ready() -> void:
	%Again.pressed.connect(App.restart_match)
	%NewMap.pressed.connect(func() -> void:
		App.match_config.seed = 0
		App.restart_match())
	%Setup.pressed.connect(App.go_to_setup)
	%Menu.pressed.connect(App.go_to_main_menu)


## Muestra el resultado con una animación de entrada.
func show_result(title: String, color: Color, details: String) -> void:
	%Title.text = title
	%Title.add_theme_color_override("font_color", color)
	%Details.text = details
	var panel: Control = %Panel
	panel.pivot_offset = panel.size / 2.0
	panel.scale = Vector2(0.6, 0.6)
	panel.modulate.a = 0.0
	var t := create_tween().set_parallel(true)
	t.tween_property(panel, "scale", Vector2.ONE, 0.45).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	t.tween_property(panel, "modulate:a", 1.0, 0.25)
	%Again.grab_focus.call_deferred()
