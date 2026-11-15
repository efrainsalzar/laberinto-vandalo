extends Control
## Configurar partida: dificultad (o personalizado), semilla y VISTA PREVIA del mapa en vivo.
## El mapa que se ve es el que se juega (se guarda su semilla en la MatchConfig).
## Ver docs/ARQUITECTURA.md §13.

const DIFFICULTIES := [
	[MazeProfile.DIFFICULTY_EASY, "Fácil", "Pasillos largos y abiertos, casi sin callejones y con muchos atajos. Para aprender."],
	[MazeProfile.DIFFICULTY_NORMAL, "Normal", "Equilibrado: pasillos largos y ramificaciones, algunos callejones."],
	[MazeProfile.DIFFICULTY_HARD, "Difícil", "Grande y uniforme: muchos cruces y callejones, pocos atajos."],
	[MazeProfile.DIFFICULTY_EXTREME, "Extremo", "Enorme, pasillos estrechos y muy ramificado. Fácil perderse."],
	[MazeProfile.DIFFICULTY_CUSTOM, "Personalizado", "Elige tú el algoritmo, el tamaño y la forma del laberinto."],
]
const ALGORITHMS := ["Backtracking", "Growing Tree", "Prim", "Kruskal", "Wilson", "Hunt-and-Kill"]
const PREVIEW_DELAY := 0.15  # espera tras un cambio antes de regenerar (evita trabar al arrastrar)

var _config: MatchConfig
var _profile: MazeProfile
var _preview_seed := 0
var _refresh_pending := false
var _difficulty_group := ButtonGroup.new()


func _ready() -> void:
	_config = App.match_config.duplicate()
	_profile = _config.profile.duplicate() if _config.profile else load("res://data/maps/normal.tres").duplicate()
	_config.profile = _profile
	_build_difficulty_buttons()
	_build_custom_controls()
	%Seed.text = str(_config.seed) if _config.seed != 0 else ""
	%Seed.text_submitted.connect(func(_t: String) -> void: _request_preview())
	%Seed.focus_exited.connect(_request_preview)
	%RandomSeed.pressed.connect(func() -> void:
		%Seed.text = ""
		_request_preview())
	%Back.pressed.connect(App.go_to_main_menu)
	%Start.pressed.connect(_start)
	_select_difficulty(_profile.difficulty)
	_refresh_preview()
	%Start.grab_focus.call_deferred()


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_cancel"):
		App.go_to_main_menu()


# --- Dificultad ------------------------------------------------------------------------------

func _build_difficulty_buttons() -> void:
	for entry in DIFFICULTIES:
		var b := Button.new()
		b.text = entry[1]
		b.toggle_mode = true
		b.button_group = _difficulty_group
		b.pressed.connect(_select_difficulty.bind(entry[0]))
		b.set_meta("difficulty", entry[0])
		%Difficulty.add_child(b)


func _select_difficulty(difficulty: int) -> void:
	if difficulty == MazeProfile.DIFFICULTY_CUSTOM and _profile.difficulty != MazeProfile.DIFFICULTY_CUSTOM:
		_profile.load_preset_values(_profile.difficulty)  # punto de partida: el nivel que estaba
		_sync_custom_controls()
	_profile.difficulty = difficulty
	for b: Button in %Difficulty.get_children():
		b.set_pressed_no_signal(b.get_meta("difficulty") == difficulty)
	for entry in DIFFICULTIES:
		if entry[0] == difficulty:
			%Description.text = entry[2]
	%Custom.visible = difficulty == MazeProfile.DIFFICULTY_CUSTOM
	_request_preview()


# --- Personalizado: controles generados a partir de una tabla --------------------------------

var _custom_rows := {}  # propiedad -> {slider, value_label, fmt}


func _build_custom_controls() -> void:
	var algo_row := HBoxContainer.new()
	var algo_label := Label.new()
	algo_label.text = "Algoritmo"
	algo_label.custom_minimum_size.x = 170
	var algo := OptionButton.new()
	algo.name = "Algorithm"
	for i in ALGORITHMS.size():
		algo.add_item(ALGORITHMS[i], i)
	algo.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	algo.item_selected.connect(func(i: int) -> void:
		_profile.algorithm = i
		_request_preview())
	algo_row.add_child(algo_label)
	algo_row.add_child(algo)
	%Custom.add_child(algo_row)
	# [propiedad, etiqueta, mín, máx, paso, formato]
	for spec in [
		["cells_width", "Ancho (celdas)", 6, 60, 1, "%d"],
		["cells_height", "Alto (celdas)", 5, 40, 1, "%d"],
		["corridor_width", "Ancho de pasillo", 1, 5, 1, "%d"],
		["loop_chance", "Atajos (bucles)", 0.0, 0.5, 0.01, "%d%%"],
		["braid_chance", "Quitar callejones", 0.0, 1.0, 0.05, "%d%%"],
		["growing_tree_bias", "Growing Tree: largo", 0.0, 1.0, 0.05, "%d%%"],
		["vault_radius", "Tamaño de bóveda", 2, 8, 1, "%d"],
		["pickup_count", "Objetos", 0, 200, 1, "%d"],
	]:
		_add_slider_row(spec[0], spec[1], spec[2], spec[3], spec[4], spec[5])
	_sync_custom_controls()


func _add_slider_row(prop: String, text: String, min_v: float, max_v: float, step: float, fmt: String) -> void:
	var row := HBoxContainer.new()
	var label := Label.new()
	label.text = text
	label.custom_minimum_size.x = 170
	label.add_theme_font_size_override("font_size", 15)
	var slider := HSlider.new()
	slider.min_value = min_v
	slider.max_value = max_v
	slider.step = step
	slider.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	slider.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	var value_label := Label.new()
	value_label.custom_minimum_size.x = 52
	value_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	value_label.add_theme_font_size_override("font_size", 15)
	slider.value_changed.connect(func(v: float) -> void:
		_profile.set(prop, int(v) if step >= 1 else v)
		value_label.text = _format(fmt, v)
		_request_preview())
	row.add_child(label)
	row.add_child(slider)
	row.add_child(value_label)
	%Custom.add_child(row)
	_custom_rows[prop] = {"slider": slider, "label": value_label, "fmt": fmt}


func _sync_custom_controls() -> void:
	for row in %Custom.get_children():
		var algo := row.get_node_or_null("Algorithm") as OptionButton
		if algo:
			algo.select(_profile.algorithm)
	for prop in _custom_rows:
		var r: Dictionary = _custom_rows[prop]
		var v: float = _profile.get(prop)
		r.slider.set_value_no_signal(v)
		r.label.text = _format(r.fmt, v)


func _format(fmt: String, v: float) -> String:
	return fmt % (v * 100.0) if fmt.contains("%%") else fmt % v


# --- Vista previa ---------------------------------------------------------------------------

func _request_preview() -> void:
	if _refresh_pending:
		return
	_refresh_pending = true
	await get_tree().create_timer(PREVIEW_DELAY).timeout
	_refresh_pending = false
	_refresh_preview()


func _refresh_preview() -> void:
	var typed: String = %Seed.text.strip_edges()
	var wanted := int(typed) if typed.is_valid_int() else 0
	var preview: Dictionary = Maze.build_preview(_profile, wanted)
	_preview_seed = preview.seed
	%Image.texture = ImageTexture.create_from_image(preview.image)
	var m: Dictionary = preview.metrics
	var size: Vector2i = preview.size
	%Metrics.text = "%s · %d×%d casillas · callejones %d%% · cruces %d · ruta base→centro %d · objetos %d\nsemilla %d" % [
		m.algorithm, size.x, size.y, roundi(m.dead_end_ratio * 100.0), m.junctions, m.route_length,
		_profile.get_pickup_count(), _preview_seed]


func _start() -> void:
	_config.profile = _profile
	_config.seed = _preview_seed  # se juega exactamente el mapa de la vista previa
	App.start_match(_config)
