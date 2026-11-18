extends HBoxContainer
## Sección armas de la tarjeta: ranura BASE (pistola, ∞) y ranura ESPECIAL (la que se recoge del mapa).
## - La activa se resalta con el color del arma; la otra queda tenue.
## - Munición en segmentos (escopeta, bazuca: pocos tiros) o en barra (láser: batería en segundos).
## - En rojo y latiendo cuando queda un cuarto o menos.
## Escucha WeaponComponent (weapon_changed, charge_changed).

const CardStyle := preload("res://scripts/ui/player_card/card_style.gd")
const MAX_SEGMENTS := 12  # con más tiros que esto se muestra una barra


## Munición: segmentos (uno por tiro) o una barra continua.
class AmmoGauge extends Control:
	const TRACK := Color(1, 1, 1, 0.1)
	var color := Color.WHITE
	var segments := 0  # 0 = barra continua
	var fraction := 1.0

	func _init() -> void:
		custom_minimum_size = Vector2(0, 5)
		mouse_filter = Control.MOUSE_FILTER_IGNORE

	func _draw() -> void:
		if segments <= 0:
			draw_rect(Rect2(Vector2.ZERO, size), TRACK)
			draw_rect(Rect2(0, 0, size.x * clampf(fraction, 0.0, 1.0), size.y), color)
			return
		var gap := 2.0
		var w := (size.x - gap * (segments - 1)) / segments
		var lit := roundi(fraction * segments)
		for i in segments:
			draw_rect(Rect2(i * (w + gap), 0, w, size.y), color if i < lit else TRACK)


var _weapon: WeaponComponent
var _base: PanelContainer
var _base_name: Label
var _special: PanelContainer
var _special_name: Label
var _special_count: Label
var _gauge: AmmoGauge
var _low := false


func _init() -> void:
	add_theme_constant_override("separation", 8)
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func bind(participant: Participant) -> void:
	_weapon = participant.get_weapon()
	var base_parts := _slot("BASE")
	_base = base_parts.panel
	_base_name = base_parts.name
	base_parts.count.text = "∞"
	add_child(_base)
	var special_parts := _slot("ESPECIAL")
	_special = special_parts.panel
	_special.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_special_name = special_parts.name
	_special_count = special_parts.count
	_gauge = AmmoGauge.new()
	special_parts.rows.add_child(_gauge)
	add_child(_special)
	if _weapon == null:
		visible = false
		return
	_weapon.weapon_changed.connect(_on_weapon_changed.unbind(1))
	_weapon.charge_changed.connect(_on_charge_changed)
	_on_weapon_changed()


func _slot(kind: String) -> Dictionary:
	var panel := PanelContainer.new()
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var rows := VBoxContainer.new()
	rows.add_theme_constant_override("separation", 2)
	rows.add_child(CardStyle.caption(kind))
	var line := HBoxContainer.new()
	line.add_theme_constant_override("separation", 10)
	var name_label := CardStyle.label("", 14)
	name_label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	var count := CardStyle.label("", 14)
	line.add_child(name_label)
	line.add_child(count)
	rows.add_child(line)
	panel.add_child(rows)
	return {"panel": panel, "rows": rows, "name": name_label, "count": count}


func _style_slot(panel: PanelContainer, color: Color, active: bool) -> void:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(color, 0.12) if active else Color(1, 1, 1, 0.03)
	style.border_color = color if active else CardStyle.LINE_SOFT
	style.set_border_width_all(1)
	style.set_corner_radius_all(7)
	style.content_margin_left = 9
	style.content_margin_right = 9
	style.content_margin_top = 5
	style.content_margin_bottom = 6
	panel.add_theme_stylebox_override("panel", style)
	panel.modulate.a = 1.0 if active else 0.55


func _on_weapon_changed() -> void:
	var base: WeaponType = _weapon.get_weapon()
	var special: WeaponType = _weapon.get_special()
	var base_color := base.projectile_color if base else CardStyle.INK
	_base_name.text = base.get_short_name() if base else "—"
	_base_name.add_theme_color_override("font_color", base_color if special == null else CardStyle.INK)
	_style_slot(_base, base_color, special == null)
	if special:
		_special_name.text = special.get_short_name()
		_special_name.add_theme_color_override("font_color", special.projectile_color)
		_gauge.color = special.projectile_color
		_gauge.segments = special.ammo if special.ammo > 0 and special.ammo <= MAX_SEGMENTS else 0
		_gauge.visible = special.ammo > 0 or special.energy > 0.0
		_style_slot(_special, special.projectile_color, true)
		_special.pivot_offset = _special.size / 2.0
		_special.scale = Vector2(1.08, 1.08)
		create_tween().tween_property(_special, "scale", Vector2.ONE, 0.3).set_trans(Tween.TRANS_BACK)
		_on_charge_changed(_weapon.get_special_charge())
	else:
		_special_name.text = "— vacía"
		_special_name.add_theme_color_override("font_color", CardStyle.INK_FAINT)
		_special_count.text = ""
		_gauge.visible = false
		_low = false
		_style_slot(_special, CardStyle.INK_FAINT, false)


func _on_charge_changed(fraction: float) -> void:
	if not _weapon.has_special():
		return
	var energy := _weapon.get_special_energy()
	var ammo := _weapon.get_special_ammo()
	var special: WeaponType = _weapon.get_special()
	if energy >= 0.0:
		_special_count.text = ("%.1f s" % energy).replace(".", ",")
	elif ammo >= 0:
		_special_count.text = "%d/%d" % [ammo, special.ammo]
	else:
		_special_count.text = "∞"
	_gauge.fraction = fraction
	_gauge.queue_redraw()
	_low = fraction <= 0.25 and (ammo >= 0 or energy >= 0.0)
	_special_count.add_theme_color_override("font_color", CardStyle.DANGER if _low else CardStyle.INK)


func _process(_delta: float) -> void:
	# Latido de "queda poco".
	_special_count.modulate.a = (0.55 + 0.45 * absf(sin(Time.get_ticks_msec() / 260.0))) if _low else 1.0
