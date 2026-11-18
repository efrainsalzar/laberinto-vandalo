extends HFlowContainer
## Sección efectos de la tarjeta: cada efecto temporal (power-up, impulso, aturdimiento) como una ficha con un
## anillo que se vacía y los segundos que le quedan. Solo se ve si hay efectos (los permanentes ya se ven en
## la stat). Escucha StatsComponent (se consulta cada frame: pocos modificadores, es barato).
## También los escudos (§27): BURBUJA / FRONTAL / ESPEJO con su tiempo (ShieldComponent.get_shields()).

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


const SHIELD_CHIPS := {
	ShieldComponent.SHIELD_BUBBLE: ["BURBUJA", Color(0.55, 0.85, 1.0)],
	ShieldComponent.SHIELD_FRONTAL: ["FRONTAL", Color(0.45, 0.62, 1.0)],
	ShieldComponent.SHIELD_MIRROR: ["ESPEJO", Color(0.88, 0.9, 0.97)],
}

var _stats: StatsComponent
var _shields: ShieldComponent
var _chips := {}  # id de modificador -> {panel, ring, time, duration}


func _init() -> void:
	add_theme_constant_override("h_separation", 6)
	add_theme_constant_override("v_separation", 6)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	visible = false


func bind(participant: Participant) -> void:
	_stats = participant.get_stats()
	_shields = participant.get_shields()


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
	if _shields:
		for s: Dictionary in _shields.get_shields():
			var id := "shield:%d" % s.kind
			alive[id] = true
			if not _chips.has(id):
				var info: Array = SHIELD_CHIPS.get(s.kind, ["ESCUDO", Color.WHITE])
				_chips[id] = _make_chip_with(info[0], info[1], s.duration)
			var schip: Dictionary = _chips[id]
			schip.duration = maxf(s.duration, 0.001)
			schip.ring.fraction = s.remaining / schip.duration
			schip.ring.queue_redraw()
			schip.time.text = "%d s" % ceili(s.remaining)
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
	return _make_chip_with(EffectText.format(m.stat, m.op, m.value), color, m.duration)


func _make_chip_with(text: String, color: Color, duration: float) -> Dictionary:
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
	row.add_child(CardStyle.label(text, 12, color))
	var time := CardStyle.label("", 12, CardStyle.INK_DIM)
	row.add_child(time)
	panel.add_child(row)
	add_child(panel)
	panel.modulate.a = 0.0
	create_tween().tween_property(panel, "modulate:a", 1.0, 0.2)
	return {"panel": panel, "ring": ring, "time": time, "duration": maxf(duration, 0.001)}
