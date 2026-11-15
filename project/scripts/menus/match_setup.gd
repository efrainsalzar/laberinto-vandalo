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
const SIZES := [
	[1.0, "Normal", "Tamaño estándar del nivel."],
	[1.5, "Grande", "×1,5: más pasillos y una ruta más larga hasta el centro."],
	[2.0, "Enorme", "×2: partidas largas, ideal para muchos jugadores."],
	[3.0, "Gigante", "×3: un laberinto inmenso. Fácil perderse; la flecha de objetivo es tu aliada."],
]
const LIVE_LEVELS := [
	["Apagado", "Las paredes no se mueven: el mapa es fijo toda la partida."],
	["Lento", "Cada 25 s se cierra un pasaje y se abre otro, con aviso previo."],
	["Normal", "Cada 15 s cambian varios pasajes. Las rutas que conocías pueden dejar de existir."],
	["Caótico", "Cada 7 s el laberinto se transforma. Nada es seguro por mucho tiempo."],
]
const DURATION_HINTS := {
	5.0: "Partida corta: fases rápidas, hay que ir directo al trofeo.",
	8.0: "Duración estándar del torneo (VISION: reloj de 8 minutos).",
	12.0: "Partida larga: más tiempo para explorar y juntar mejoras.",
	20.0: "Muy larga: ideal para mapas Enormes o Gigantes.",
	0.0: "Sin límite: las fases avanzan, pero el tiempo nunca se acaba. Para practicar.",
}
const ALGORITHMS := ["Backtracking", "Growing Tree", "Prim", "Kruskal", "Wilson", "Hunt-and-Kill"]
const ALGORITHM_HINTS := [
	"Backtracking: pasillos muy largos y sinuosos, pocos callejones.",
	"Growing Tree: mezcla ajustable entre pasillos largos y ramificaciones (barra «Growing Tree: largo»).",
	"Prim: muy ramificado, muchos callejones cortos. Confunde.",
	"Kruskal: uniforme, con muchos cruces.",
	"Wilson: uniforme y sin sesgo: cualquier laberinto es igual de probable.",
	"Hunt-and-Kill: parecido a Backtracking, algo más ramificado.",
]
const PREVIEW_DELAY := 0.15  # espera tras un cambio antes de regenerar (evita trabar al arrastrar)

var _config: MatchConfig
var _profile: MazeProfile
var _preview_seed := 0
var _refresh_pending := false
var _difficulty_group := ButtonGroup.new()
var _size_group := ButtonGroup.new()
var _live_group := ButtonGroup.new()
var _duration_group := ButtonGroup.new()


func _ready() -> void:
	_config = App.match_config.duplicate()
	_profile = _config.profile.duplicate() if _config.profile else load("res://data/maps/normal.tres").duplicate()
	_config.profile = _profile
	_build_difficulty_buttons()
	_build_size_buttons()
	_build_live_buttons()
	_build_duration_buttons()
	_build_custom_controls()
	UiFx.hint(%Seed, "Número que define el mapa. Vacío = uno al azar. Escribe la semilla de un mapa para repetirlo o compartirlo.")
	UiFx.hint(%RandomSeed, "Genera otro mapa al azar con la misma configuración.")
	UiFx.hint(%Start, "Empieza la partida con el mapa de la vista previa.")
	UiFx.hint(%Back, "Volver al menú principal.")
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
		UiFx.hint(b, entry[2])
		%Difficulty.add_child(b)


func _build_duration_buttons() -> void:
	var custom := not MatchConfig.DURATIONS.has(_config.duration_minutes)
	for minutes: float in MatchConfig.DURATIONS:
		var b := Button.new()
		b.text = "Sin límite" if minutes == 0.0 else "%d min" % minutes
		b.toggle_mode = true
		b.button_group = _duration_group
		b.set_pressed_no_signal(not custom and _config.duration_minutes == minutes)
		b.pressed.connect(func() -> void:
			_config.duration_minutes = minutes
			%DurationCustom.visible = false)
		UiFx.hint(b, DURATION_HINTS[minutes])
		%Duration.add_child(b)
	var b := Button.new()
	b.text = "Personalizada"
	b.toggle_mode = true
	b.button_group = _duration_group
	b.set_pressed_no_signal(custom)
	UiFx.hint(b, "Elige los minutos exactos con la barra (de 1 a 60).")
	%Duration.add_child(b)
	# Barra de minutos (solo visible en "Personalizada").
	var row := HBoxContainer.new()
	var label := Label.new()
	label.text = "Minutos"
	label.custom_minimum_size.x = 170
	label.add_theme_font_size_override("font_size", 15)
	var slider := HSlider.new()
	slider.min_value = 1
	slider.max_value = 60
	slider.step = 1
	slider.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	slider.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	var value := Label.new()
	value.custom_minimum_size.x = 60
	value.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	slider.value_changed.connect(func(v: float) -> void:
		value.text = "%d min" % v
		_config.duration_minutes = v)
	slider.value = _config.duration_minutes if custom else 10.0
	value.text = "%d min" % slider.value
	UiFx.hint(slider, "Duración total de la partida. Las fases se reparten: 40% Preparación, 35% Asalto, 25% Golpe.")
	row.add_child(label)
	row.add_child(slider)
	row.add_child(value)
	%DurationCustom.add_child(row)
	%DurationCustom.visible = custom
	b.pressed.connect(func() -> void:
		_config.duration_minutes = slider.value
		%DurationCustom.visible = true)


func _build_live_buttons() -> void:
	for i in LIVE_LEVELS.size():
		var b := Button.new()
		b.text = LIVE_LEVELS[i][0]
		b.toggle_mode = true
		b.button_group = _live_group
		b.set_pressed_no_signal(_config.live_maze == i)
		b.pressed.connect(func() -> void: _config.live_maze = i)
		UiFx.hint(b, LIVE_LEVELS[i][1])
		%Live.add_child(b)


func _build_size_buttons() -> void:
	for entry in SIZES:
		var b := Button.new()
		b.text = entry[1]
		b.toggle_mode = true
		b.button_group = _size_group
		b.set_pressed_no_signal(is_equal_approx(_profile.size_scale, entry[0]))
		b.pressed.connect(func() -> void:
			_profile.size_scale = entry[0]
			_request_preview())
		UiFx.hint(b, entry[2])
		%Size.add_child(b)


func _select_difficulty(difficulty: int) -> void:
	if difficulty == MazeProfile.DIFFICULTY_CUSTOM and _profile.difficulty != MazeProfile.DIFFICULTY_CUSTOM:
		_profile.load_preset_values(_profile.difficulty)  # punto de partida: el nivel que estaba
		_sync_custom_controls()
	_profile.difficulty = difficulty
	for b: Button in %Difficulty.get_children():
		b.set_pressed_no_signal(b.get_meta("difficulty") == difficulty)
	var custom := difficulty == MazeProfile.DIFFICULTY_CUSTOM
	%Custom.visible = custom
	%Size.visible = not custom  # en personalizado el tamaño se elige con las barras
	%SizeLabel.visible = not custom
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
		UiFx.hint(algo, ALGORITHM_HINTS[i])
		_request_preview())
	UiFx.hint(algo, ALGORITHM_HINTS[_profile.algorithm])
	algo_row.add_child(algo_label)
	algo_row.add_child(algo)
	%Custom.add_child(algo_row)
	# [propiedad, etiqueta, mín, máx, paso, formato, descripción]
	for spec in [
		["cells_width", "Ancho (celdas)", 6, 160, 1, "%d", "Celdas de ancho. Cada celda es un tramo de pasillo con su pared."],
		["cells_height", "Alto (celdas)", 5, 110, 1, "%d", "Celdas de alto."],
		["corridor_width", "Ancho de pasillo", 1, 5, 1, "%d", "Casillas de ancho de cada pasillo. Más ancho = más espacio para esquivar."],
		["loop_chance", "Atajos (bucles)", 0.0, 0.5, 0.01, "%d%%", "Probabilidad de abrir paredes extra: más rutas alternativas para huir o emboscar."],
		["braid_chance", "Quitar callejones", 0.0, 1.0, 0.05, "%d%%", "Probabilidad de eliminar cada callejón sin salida. 100% = sin callejones."],
		["growing_tree_bias", "Growing Tree: largo", 0.0, 1.0, 0.05, "%d%%", "Solo para Growing Tree: alto = pasillos largos; bajo = muy ramificado."],
		["vault_radius", "Tamaño de bóveda", 2, 8, 1, "%d", "Radio de la sala central donde está el trofeo."],
		["pickup_count", "Objetos", 0, 300, 1, "%d", "Cuántos objetos hay a la vez en el mapa (reaparecen al recogerlos)."],
	]:
		_add_slider_row(spec[0], spec[1], spec[2], spec[3], spec[4], spec[5], spec[6])
	_sync_custom_controls()


func _add_slider_row(prop: String, text: String, min_v: float, max_v: float, step: float, fmt: String, hint: String) -> void:
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
	UiFx.hint(slider, hint)
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
