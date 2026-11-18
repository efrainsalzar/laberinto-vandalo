extends VBoxContainer
## Sección stats de la tarjeta: VEL y VIS con barra hasta el tope de mejoras.
## - Barra llena = valor base + tope de mejoras permanentes (StatsProfile.*_bonus_max).
## - Parte sólida = lo permanente; parte rayada = lo que suma un efecto temporal.
## - Al lado del número, el multiplicador activo (×0,85 al llevar el trofeo, ×1,5 de un power-up).
## - "MÁX" en dorado cuando las mejoras permanentes llegaron al tope.
## - El número cuenta hasta el valor nuevo y destella verde (sube) o rojo (baja).
## Escucha StatsComponent (stat_changed, modifiers_changed).

const CardStyle := preload("res://scripts/ui/player_card/card_style.gd")
const COUNT_TIME := 0.35
const ROWS := [
	{"stat": StatsComponent.STAT_SPEED, "key": "VEL", "color": Color(0.45, 0.9, 0.45), "decimals": 0},
	{"stat": StatsComponent.STAT_VISION, "key": "VIS", "color": Color(1.0, 0.88, 0.4), "decimals": 1},
]


## Barra: parte sólida (permanente) + parte rayada (temporal).
class StatBar extends Control:
	const TRACK := Color(1, 1, 1, 0.08)
	var color := Color.WHITE
	var solid := 0.0
	var extra := 0.0

	func _init() -> void:
		custom_minimum_size = Vector2(0, 4)
		size_flags_horizontal = Control.SIZE_EXPAND_FILL
		size_flags_vertical = Control.SIZE_SHRINK_CENTER
		mouse_filter = Control.MOUSE_FILTER_IGNORE

	func _draw() -> void:
		var h := size.y
		draw_rect(Rect2(Vector2.ZERO, size), TRACK)
		var w_solid := size.x * clampf(solid, 0.0, 1.0)
		draw_rect(Rect2(0, 0, w_solid, h), color)
		var w_extra := size.x * clampf(extra, 0.0, 1.0 - clampf(solid, 0.0, 1.0))
		var x := w_solid
		while x < w_solid + w_extra:  # rayas de 3 px con huecos de 2 px
			draw_rect(Rect2(x, 0, minf(3.0, w_solid + w_extra - x), h), Color(color, 0.85))
			x += 5.0


var _stats: StatsComponent
var _rows := {}  # stat -> {bar, value, mult, shown, tween, decimals}


func _init() -> void:
	add_theme_constant_override("separation", 3)
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func bind(participant: Participant) -> void:
	_stats = participant.get_stats()
	for row: Dictionary in ROWS:
		var line := HBoxContainer.new()
		line.add_theme_constant_override("separation", 8)
		var key := CardStyle.caption(row.key, CardStyle.INK_DIM)
		key.custom_minimum_size.x = 24
		line.add_child(key)
		var bar := StatBar.new()
		bar.color = row.color
		line.add_child(bar)
		var value := CardStyle.label("", 13)
		value.custom_minimum_size.x = 34
		value.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		line.add_child(value)
		var mult := CardStyle.label("", 11, CardStyle.INK_FAINT)
		mult.custom_minimum_size.x = 32
		line.add_child(mult)
		add_child(line)
		_rows[row.stat] = {"bar": bar, "value": value, "mult": mult, "shown": 0.0, "tween": null, "decimals": row.decimals}
	if _stats == null:
		visible = false
		return
	_stats.stat_changed.connect(_on_stat_changed)
	_stats.modifiers_changed.connect(_refresh_all)
	for stat in _rows:
		_rows[stat].shown = _stats.get_stat(stat)
	_refresh_all()


func _refresh_all() -> void:
	for stat in _rows:
		_refresh(stat)


func _on_stat_changed(stat: int, value: float) -> void:
	if not _rows.has(stat):
		return
	var row: Dictionary = _rows[stat]
	var from: float = row.shown
	if is_equal_approx(from, value):
		_refresh(stat)
		return
	if row.tween:
		row.tween.kill()
	var label: Label = row.value
	label.add_theme_color_override("font_color", Color(0.45, 1.0, 0.6) if value > from else CardStyle.DANGER)
	label.pivot_offset = label.size / 2.0
	label.scale = Vector2(1.15, 1.15)
	var tween := create_tween().set_parallel(true)
	tween.tween_method(func(v: float) -> void:
		row.shown = v
		_refresh(stat), from, value, COUNT_TIME)
	tween.tween_property(label, "scale", Vector2.ONE, COUNT_TIME).set_trans(Tween.TRANS_BACK)
	tween.chain().tween_callback(func() -> void:
		label.remove_theme_color_override("font_color")
		_refresh(stat))
	row.tween = tween


## Redibuja una fila: valor mostrado, barra (permanente + temporal), multiplicador y MÁX.
func _refresh(stat: int) -> void:
	var row: Dictionary = _rows[stat]
	var profile: StatsProfile = _stats.get_profile()
	var base := _stats.get_stat(stat)
	var bonus_max := 0.0
	if profile:
		base = profile.speed if stat == StatsComponent.STAT_SPEED else profile.vision
		bonus_max = profile.speed_bonus_max if stat == StatsComponent.STAT_SPEED else profile.vision_bonus_max
	var permanent := base
	var temp_add := 0.0
	var mult := 1.0
	for m: Dictionary in _stats.get_modifiers():
		if m.stat != stat:
			continue
		if m.op == StatsComponent.OP_MULTIPLY:
			mult *= m.value
		elif m.duration < 0.0:
			permanent += m.value
		else:
			temp_add += m.value
	var cap := maxf(base + bonus_max, 0.001)
	var current := _stats.get_stat(stat)
	var bar: StatBar = row.bar
	if current >= permanent:
		bar.solid = permanent / cap
		bar.extra = (current - permanent) / cap
	else:
		bar.solid = current / cap  # un multiplicador menor a 1 (portador del trofeo) achica la barra
		bar.extra = 0.0
	bar.queue_redraw()
	var maxed := _stats.is_bonus_maxed(stat)
	var shown: float = row.shown
	var label: Label = row.value
	label.text = (str(roundi(shown)) if row.decimals == 0 else "%.1f" % shown)
	if maxed and not label.has_theme_color_override("font_color"):
		label.add_theme_color_override("font_color", CardStyle.GOLD)
	var mult_label: Label = row.mult
	if not is_equal_approx(mult, 1.0):
		mult_label.text = CardStyle.multiplier_text(mult)
		mult_label.add_theme_color_override("font_color", CardStyle.OK if mult > 1.0 else CardStyle.DANGER)
	elif temp_add != 0.0:
		mult_label.text = "+%s" % str(snappedf(temp_add, 0.1))
		mult_label.add_theme_color_override("font_color", CardStyle.OK)
	else:
		mult_label.text = "MÁX" if maxed else ""
		mult_label.add_theme_color_override("font_color", CardStyle.GOLD)
