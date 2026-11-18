extends HBoxContainer
## Sección salud de la tarjeta (Etapa 4, §25): "SAL" + barra + número.
## - La barra cambia de color según cuánta queda (verde → amarillo → rojo bajo 30).
## - Al recibir daño, el trozo perdido queda un instante en blanco y se vacía detrás (se lee cuánto dolió).
## - El número destella rojo al bajar y verde al subir (curar, pieza 2).
## Sin corazones: las vidas son infinitas por ahora.
## Escucha HealthComponent (health_changed); nunca modifica el juego.

const CardStyle := preload("res://scripts/ui/player_card/card_style.gd")
const TRAIL_DELAY := 0.25
const TRAIL_TIME := 0.35


## Barra de salud: parte llena + rastro blanco de lo que se acaba de perder.
class HealthBar extends Control:
	const TRACK := Color(1, 1, 1, 0.08)
	const LOW_RATIO := 0.3
	const RED := Color(1.0, 0.4, 0.35)
	const YELLOW := Color(1.0, 0.82, 0.3)
	const GREEN := Color(0.45, 0.9, 0.45)
	var fill := 1.0
	var trail := 1.0  # >= fill mientras se vacía el rastro

	func _init() -> void:
		custom_minimum_size = Vector2(0, 6)
		size_flags_horizontal = Control.SIZE_EXPAND_FILL
		size_flags_vertical = Control.SIZE_SHRINK_CENTER
		mouse_filter = Control.MOUSE_FILTER_IGNORE

	func _draw() -> void:
		draw_rect(Rect2(Vector2.ZERO, size), TRACK)
		var f := clampf(fill, 0.0, 1.0)
		var t := clampf(trail, f, 1.0)
		if t > f:
			draw_rect(Rect2(size.x * f, 0, size.x * (t - f), size.y), Color(1, 1, 1, 0.75))
		draw_rect(Rect2(0, 0, size.x * f, size.y), color_for(f))

	static func color_for(ratio: float) -> Color:
		if ratio < LOW_RATIO:
			return RED
		if ratio < 0.6:
			return YELLOW
		return GREEN


var _health: HealthComponent
var _bar: HealthBar
var _value: Label
var _shown := -1.0
var _trail_tween: Tween
var _flash_tween: Tween


func _init() -> void:
	add_theme_constant_override("separation", 8)
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func bind(participant: Participant) -> void:
	_health = participant.get_health()
	var key := CardStyle.caption("SAL", CardStyle.INK_DIM)
	key.custom_minimum_size.x = 24
	add_child(key)
	_bar = HealthBar.new()
	add_child(_bar)
	_value = CardStyle.label("", 13)
	_value.custom_minimum_size.x = 34
	_value.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	add_child(_value)
	var spacer := Control.new()  # alinea el número con los de VEL y VIS (que tienen columna de multiplicador)
	spacer.custom_minimum_size.x = 32
	spacer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(spacer)
	if _health == null:
		visible = false
		return
	_health.health_changed.connect(_on_health_changed)
	_on_health_changed(_health.get_health(), _health.get_max_health())


func _on_health_changed(current: float, max_value: float) -> void:
	var ratio := current / maxf(max_value, 0.001)
	var before := _bar.fill
	_bar.fill = ratio
	_value.text = str(ceili(current))
	if _trail_tween:
		_trail_tween.kill()
	if ratio < before and _shown >= 0.0:
		_trail_tween = create_tween()
		_trail_tween.tween_interval(TRAIL_DELAY)
		_trail_tween.tween_method(func(v: float) -> void:
			_bar.trail = v
			_bar.queue_redraw(), _bar.trail, ratio, TRAIL_TIME)
		_flash(CardStyle.DANGER)
	else:
		_bar.trail = ratio
		if _shown >= 0.0 and current > _shown:
			_flash(CardStyle.OK)
	_shown = current
	_bar.queue_redraw()


func _flash(color: Color) -> void:
	if _flash_tween:
		_flash_tween.kill()
	_value.add_theme_color_override("font_color", color)
	_value.pivot_offset = _value.size / 2.0
	_value.scale = Vector2(1.18, 1.18)
	_flash_tween = create_tween()
	_flash_tween.tween_property(_value, "scale", Vector2.ONE, 0.3).set_trans(Tween.TRANS_BACK)
	_flash_tween.tween_callback(func() -> void: _value.remove_theme_color_override("font_color"))
