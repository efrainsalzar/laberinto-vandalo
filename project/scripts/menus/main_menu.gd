extends Control
## Menú principal. Fondo: un laberinto real que se desplaza lentamente.
## Los botones solo piden a App el cambio de pantalla (App decide el flujo).

const SCROLL_SPEED := Vector2(14.0, 8.0)
const BG_SCALE := 14.0  # px de pantalla por casilla del fondo

var _settings: Control
var _bg_size := Vector2.ZERO
var _bg_dir := Vector2.ONE


func _ready() -> void:
	%Version.text = "v%s · Godot %s" % [ProjectSettings.get_setting("application/config/version"), Engine.get_version_info().string]
	%Play.pressed.connect(App.go_to_setup)
	%QuickPlay.pressed.connect(func() -> void: App.start_match(MatchConfig.create_default()))
	%Options.pressed.connect(_open_settings)
	%Quit.pressed.connect(App.quit_game)
	%Stress.pressed.connect(func() -> void: App.start_match(MatchConfig.create_stress()))
	UiFx.hint(%Stress, "Mide el rendimiento en el peor caso: mapa Gigante, laberinto Caótico, 40 entidades y cambios de varias fuentes a la vez. Al terminar (30 s) muestra un informe.")
	UiFx.hint(%Play, "Elige la dificultad y el tamaño del mapa, con vista previa, y empieza.")
	UiFx.hint(%QuickPlay, "Empieza ya en un mapa Normal al azar.")
	UiFx.hint(%Options, "Pantalla, rendimiento, cámara y teclas de prueba.")
	UiFx.hint(%Quit, "Cerrar el juego.")
	_make_background()
	# Ejecución automática: godot --path project -- --stress [--quit-after-stress]
	if "--stress" in OS.get_cmdline_user_args() and not App.has_meta("stress_started"):
		App.set_meta("stress_started", true)
		App.start_match.call_deferred(MatchConfig.create_stress())
	_intro()
	%Play.grab_focus.call_deferred()


func _process(delta: float) -> void:
	# Desplazamiento lento del fondo, rebotando en los bordes.
	var bg: TextureRect = %Background
	bg.position += SCROLL_SPEED * _bg_dir * delta
	var min_pos := size - _bg_size
	if bg.position.x > 0.0 or bg.position.x < min_pos.x:
		_bg_dir.x = -_bg_dir.x
	if bg.position.y > 0.0 or bg.position.y < min_pos.y:
		_bg_dir.y = -_bg_dir.y
	bg.position = bg.position.clamp(min_pos, Vector2.ZERO)


func _make_background() -> void:
	var profile: MazeProfile = load("res://data/maps/dificil.tres")
	var preview: Dictionary = Maze.build_preview(profile, 0)
	var bg: TextureRect = %Background
	bg.texture = ImageTexture.create_from_image(preview.image)
	_bg_size = Vector2(preview.size) * BG_SCALE
	bg.size = _bg_size
	bg.position = -(_bg_size - size) / 2.0


func _intro() -> void:
	var rows: Array[Node] = $Center/Rows.get_children()
	for i in rows.size():
		var c := rows[i] as Control
		c.modulate.a = 0.0
		var t := create_tween()
		t.tween_interval(0.06 * i)
		t.tween_property(c, "modulate:a", 1.0, 0.3)


func _open_settings() -> void:
	if _settings != null:
		return
	_settings = load("res://scenes/menus/settings_menu.tscn").instantiate()
	add_child(_settings)
	_settings.closed.connect(func() -> void:
		_settings = null
		%Options.grab_focus())
