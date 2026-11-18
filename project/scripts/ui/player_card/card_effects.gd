extends HFlowContainer
## Sección efectos de la tarjeta: cada efecto temporal (power-up, impulso, aturdimiento) como una ficha con un
## anillo que se vacía y los segundos que le quedan. Solo se ve si hay efectos (los permanentes ya se ven en
## la stat). Escucha StatsComponent (se consulta cada frame: pocos modificadores, es barato).

const CardStyle := preload("res://scripts/ui/player_card/card_style.gd")


## Anillo que se vacía con el tiempo que le queda al efecto.
class TimerRing extends Control:
	const TRACK := Color(1, 1, 1, 0.12)
	var color := Color.WHITE
	var fraction := 1.0

	func _init() -> void:
		custom_minimum_size = Vector2(16, 16)
		size_flags_vertical = Control.SIZE_SHRINK_CENTER
		mouse_filter = Control.MOUSE_FILTER_IGNORE

	func _draw() -> void:
		var c := size / 2.0
		var r := minf(size.x, size.y) / 2.0 - 1.5
		draw_arc(c, r, 0.0, TAU, 24, TRACK, 2.5, true)
		draw_arc(c, r, -PI / 2.0, -PI / 2.0 + TAU * clampf(fraction, 0.0, 1.0), 24, color, 2.5, true)


var _stats: StatsComponent
var _chips := {}  # id de modificador -> {panel, ring, time, duration}


func _init() -> void:
	add_theme_constant_override("h_separation", 6)
	add_theme_constant_override("v_separation", 6)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	visible = false


func bind(participant: Participant) -> void:
	_stats = participant.get_stats()


func _process(_delta: float) -> void:
	if _stats == null:
		return
	var alive := {}
	for m: Dictionary in _stats.get_modifiers():
		if m.duration < 0.0:
			continue
		alive[m.id] = true
		if not _chips.has(m.id):
			_chips[m.id] = _make_chip(m)
		var chip: Dictionary = _chips[m.id]
		chip.ring.fraction = m.remaining / chip.duration
		chip.ring.queue_redraw()
		chip.time.text = "%d s" % ceili(m.remaining)
	for id in _chips.keys():
		if not alive.has(id):
			var panel: Control = _chips[id].panel
			_chips.erase(id)
			var t := create_tween()
			t.tween_property(panel, "modulate:a", 0.0, 0.2)
			t.tween_callback(panel.queue_free)
	visible = not _chips.is_empty()


func _make_chip(m: Dictionary) -> Dictionary:
	var color: Color = EffectText.STAT_COLORS.get(m.stat, Color.WHITE)
	var panel := PanelContainer.new()
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var style := StyleBoxFlat.new()
	style.bg_color = Color(1, 1, 1, 0.05)
	style.border_color = CardStyle.LINE_SOFT
	style.set_border_width_all(1)
	style.set_corner_radius_all(99)
	style.content_margin_left = 4
	style.content_margin_right = 9
	style.content_margin_top = 2
	style.content_margin_bottom = 2
	panel.add_theme_stylebox_override("panel", style)
	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 5)
	var ring := TimerRing.new()
	ring.color = color
	row.add_child(ring)
	row.add_child(CardStyle.label(EffectText.format(m.stat, m.op, m.value), 12, color))
	var time := CardStyle.label("", 12, CardStyle.INK_DIM)
	row.add_child(time)
	panel.add_child(row)
	add_child(panel)
	panel.modulate.a = 0.0
	create_tween().tween_property(panel, "modulate:a", 1.0, 0.2)
	return {"panel": panel, "ring": ring, "time": time, "duration": maxf(m.duration, 0.001)}
