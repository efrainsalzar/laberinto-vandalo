extends HBoxContainer
## Sección armas de la tarjeta compacta: UNA línea con el arma activa.
## - Con arma especial: ✦ NOMBRE (con su color) · munición en segmentos (pocos tiros) o barra (batería) · cuenta.
## - Sin especial: "PISTOLA ∞" en tenue.
## - La cuenta se pone roja y late cuando queda un cuarto o menos.
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
		custom_minimum_size = Vector2(0, 4)
		size_flags_horizontal = Control.SIZE_EXPAND_FILL
		size_flags_vertical = Control.SIZE_SHRINK_CENTER
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
var _name: Label
var _gauge: AmmoGauge
var _count: Label
var _low := false


func _init() -> void:
	add_theme_constant_override("separation", 8)
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func bind(participant: Participant) -> void:
	_weapon = participant.get_weapon()
	_name = CardStyle.label("", 12)
	add_child(_name)
	_gauge = AmmoGauge.new()
	add_child(_gauge)
	_count = CardStyle.label("", 12)
	add_child(_count)
	if _weapon == null:
		visible = false
		return
	_weapon.weapon_changed.connect(_on_weapon_changed.unbind(1))
	_weapon.charge_changed.connect(_on_charge_changed)
	_on_weapon_changed()


func _on_weapon_changed() -> void:
	var special: WeaponType = _weapon.get_special()
	if special:
		_name.text = "✦ " + special.get_short_name()
		_name.add_theme_color_override("font_color", special.projectile_color)
		_gauge.color = special.projectile_color
		_gauge.segments = special.ammo if special.ammo > 0 and special.ammo <= MAX_SEGMENTS else 0
		_gauge.visible = special.ammo > 0 or special.energy > 0.0
		_on_charge_changed(_weapon.get_special_charge())
		pivot_offset = size / 2.0
		scale = Vector2(1.08, 1.08)
		create_tween().tween_property(self, "scale", Vector2.ONE, 0.3).set_trans(Tween.TRANS_BACK)
	else:
		var base: WeaponType = _weapon.get_weapon()
		_name.text = base.get_short_name() if base else "—"
		_name.add_theme_color_override("font_color", CardStyle.INK_DIM)
		_gauge.visible = true
		_gauge.segments = 0
		_gauge.fraction = 0.0  # sin barra: solo la pista tenue, para que la línea no salte de largo
		_gauge.queue_redraw()
		_count.text = "∞"
		_count.add_theme_color_override("font_color", CardStyle.INK_DIM)
		_low = false


func _on_charge_changed(fraction: float) -> void:
	if not _weapon.has_special():
		return
	var energy := _weapon.get_special_energy()
	var ammo := _weapon.get_special_ammo()
	var special: WeaponType = _weapon.get_special()
	if energy >= 0.0:
		_count.text = ("%.1f s" % energy).replace(".", ",")
	elif ammo >= 0:
		_count.text = "%d/%d" % [ammo, special.ammo]
	else:
		_count.text = "∞"
	_gauge.fraction = fraction
	_gauge.queue_redraw()
	_low = fraction <= 0.25 and (ammo >= 0 or energy >= 0.0)
	_count.add_theme_color_override("font_color", CardStyle.DANGER if _low else CardStyle.INK)


func _process(_delta: float) -> void:
	# Latido de "queda poco".
	_count.modulate.a = (0.55 + 0.45 * absf(sin(Time.get_ticks_msec() / 260.0))) if _low else 1.0
