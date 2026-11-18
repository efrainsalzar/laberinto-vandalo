class_name WeaponPanel
extends PanelContainer
## Panel de armas del HUD (abajo al centro, en el borde: no tapa el centro de la pantalla).
## Dos ranuras: la PISTOLA (fija, munición infinita) y la ESPECIAL (la que se recoge del mapa), con su munición
## en números y en barra. La activa se resalta con el color del arma.
## Solo vista: escucha las señales del WeaponComponent (Observer), no decide nada. Ver docs/arquitectura/23-*.md.

const DIM := Color(1, 1, 1, 0.35)
const EMPTY_TEXT := "—  sin arma especial"

var _weapon: WeaponComponent
var _base_name: Label
var _special_name: Label
var _special_ammo: Label
var _special_bar: ProgressBar
var _special_box: VBoxContainer
var _base_box: VBoxContainer


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.04, 0.03, 0.07, 0.72)
	style.border_color = Color(0.45, 0.4, 0.65, 0.35)
	style.set_border_width_all(1)
	style.set_corner_radius_all(6)
	style.content_margin_left = 14
	style.content_margin_right = 14
	style.content_margin_top = 6
	style.content_margin_bottom = 6
	add_theme_stylebox_override("panel", style)

	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 18)
	add_child(row)

	_base_box = VBoxContainer.new()
	_base_name = _label(16)
	_base_box.add_child(_base_name)
	var infinite := _label(12)
	infinite.text = "∞"
	_base_box.add_child(infinite)
	row.add_child(_base_box)

	var sep := VSeparator.new()
	row.add_child(sep)

	_special_box = VBoxContainer.new()
	_special_box.custom_minimum_size.x = 170
	var top := HBoxContainer.new()
	_special_name = _label(16)
	_special_name.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_special_ammo = _label(16)
	top.add_child(_special_name)
	top.add_child(_special_ammo)
	_special_box.add_child(top)
	_special_bar = ProgressBar.new()
	_special_bar.show_percentage = false
	_special_bar.custom_minimum_size = Vector2(0, 6)
	_special_bar.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_special_box.add_child(_special_bar)
	row.add_child(_special_box)
	# Abajo al centro (después de armar el contenido, para usar su tamaño mínimo). Crece hacia los lados y hacia arriba.
	grow_horizontal = Control.GROW_DIRECTION_BOTH
	grow_vertical = Control.GROW_DIRECTION_BEGIN
	set_anchors_and_offsets_preset(Control.PRESET_CENTER_BOTTOM, Control.PRESET_MODE_MINSIZE, 10)


## Conecta el panel a las armas del participante de esta pantalla.
func bind(weapon: WeaponComponent) -> void:
	_weapon = weapon
	if _weapon == null:
		visible = false
		return
	_weapon.weapon_changed.connect(_on_weapon_changed.unbind(1))
	_weapon.charge_changed.connect(_on_charge_changed)
	_on_weapon_changed()


func _on_weapon_changed() -> void:
	var base: WeaponType = _weapon.get_weapon()
	_base_name.text = base.get_short_name() if base else "—"
	var special: WeaponType = _weapon.get_special()
	if special:
		_special_name.text = special.get_short_name()
		_special_name.add_theme_color_override("font_color", special.projectile_color)
		_special_bar.max_value = 1.0
		_special_bar.step = 0.0
		_special_bar.modulate = special.projectile_color
		_special_bar.visible = special.ammo > 0 or special.energy > 0.0
		_on_charge_changed(_weapon.get_special_charge())
		_special_box.modulate = Color.WHITE
		_base_box.modulate = DIM
		_pulse(_special_box)
	else:
		_special_name.text = EMPTY_TEXT
		_special_name.remove_theme_color_override("font_color")
		_special_ammo.text = ""
		_special_bar.visible = false
		_special_box.modulate = DIM
		_base_box.modulate = Color.WHITE
		if base:
			_base_name.add_theme_color_override("font_color", base.projectile_color)


func _on_charge_changed(fraction: float) -> void:
	if _weapon == null or not _weapon.has_special():
		return
	var energy := _weapon.get_special_energy()
	var ammo := _weapon.get_special_ammo()
	if energy >= 0.0:
		_special_ammo.text = "%.1f s" % energy
	else:
		_special_ammo.text = "∞" if ammo < 0 else str(ammo)
	_special_bar.value = fraction
	_special_ammo.add_theme_color_override("font_color", Color(1.0, 0.4, 0.35) if fraction <= 0.25 else Color.WHITE)


func _pulse(control: Control) -> void:
	control.pivot_offset = control.size / 2.0
	control.scale = Vector2(1.15, 1.15)
	create_tween().tween_property(control, "scale", Vector2.ONE, 0.3).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)


func _label(font_size: int) -> Label:
	var label := Label.new()
	label.add_theme_font_size_override("font_size", font_size)
	label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return label
