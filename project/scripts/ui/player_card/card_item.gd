extends PanelContainer
## Sección objeto de la tarjeta (§28): la ranura de la mochila. "[Q] BOMBA" con la cantidad en puntos (●●○)
## y "×2" a la derecha; sin objeto, "— vacía" tenue. Al recoger o usar, la ranura da un saltito.
## Escucha ItemComponent (item_changed); nunca modifica el juego.

const CardStyle := preload("res://scripts/ui/player_card/card_style.gd")

var _items: ItemComponent
var _name: Label
var _count: Label
var _dots: Label


func _init() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func bind(participant: Participant) -> void:
	_items = participant.get_items()
	var rows := VBoxContainer.new()
	rows.add_theme_constant_override("separation", 2)
	rows.mouse_filter = Control.MOUSE_FILTER_IGNORE
	rows.add_child(CardStyle.caption("OBJETO · Q / CLIC DERECHO"))
	var line := HBoxContainer.new()
	line.add_theme_constant_override("separation", 10)
	_name = CardStyle.label("", 14)
	_name.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	line.add_child(_name)
	_dots = CardStyle.label("", 12)
	line.add_child(_dots)
	_count = CardStyle.label("", 14)
	line.add_child(_count)
	rows.add_child(line)
	add_child(rows)
	if _items == null:
		visible = false
		return
	_items.item_changed.connect(_on_item_changed)
	_on_item_changed(_items.get_item(), _items.get_count(), false)


func _on_item_changed(item: UsableItem, count: int, animate := true) -> void:
	var style := StyleBoxFlat.new()
	style.set_border_width_all(1)
	style.set_corner_radius_all(7)
	style.content_margin_left = 9
	style.content_margin_right = 9
	style.content_margin_top = 5
	style.content_margin_bottom = 6
	if item == null or count <= 0:
		_name.text = "— vacía"
		_name.add_theme_color_override("font_color", CardStyle.INK_FAINT)
		_dots.text = ""
		_count.text = ""
		style.bg_color = Color(1, 1, 1, 0.03)
		style.border_color = CardStyle.LINE_SOFT
	else:
		_name.text = item.get_short_name()
		_name.add_theme_color_override("font_color", item.color)
		_dots.text = "●".repeat(count) + "○".repeat(maxi(0, item.max_carry - count))
		_dots.add_theme_color_override("font_color", item.color)
		_count.text = "×%d" % count
		style.bg_color = Color(item.color, 0.12)
		style.border_color = item.color
	add_theme_stylebox_override("panel", style)
	if animate:
		pivot_offset = size / 2.0
		scale = Vector2(1.06, 1.06)
		create_tween().tween_property(self, "scale", Vector2.ONE, 0.25).set_trans(Tween.TRANS_BACK)
