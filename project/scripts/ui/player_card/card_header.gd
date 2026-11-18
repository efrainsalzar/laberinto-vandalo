extends HBoxContainer
## Sección cabecera de la tarjeta, en UNA línea: avatar con anillo de estado · nombre · estado · ♛.
## - Anillo: lleno con tu color (en juego), celeste que se vacía (protegido), rojo que se vacía (eliminado,
##   cuenta la espera para reaparecer). En la Etapa 4 mostrará la salud.
## - Estado: EN JUEGO / PROTEGIDO / ELIMINADO · N S. Corona dorada cuando llevas el trofeo.
## Escucha LifeComponent (se consulta cada frame: es barato) y los eventos del trofeo.

const CardStyle := preload("res://scripts/ui/player_card/card_style.gd")


## Avatar dibujado: anillo de estado + cuerpo con el color del participante + "ojo".
class Avatar extends Control:
	const TRACK := Color(1, 1, 1, 0.08)
	var color := Color.WHITE
	var ring_color := Color.WHITE
	var ring_fraction := 1.0

	func _init() -> void:
		custom_minimum_size = Vector2(28, 28)
		size_flags_vertical = Control.SIZE_SHRINK_CENTER
		mouse_filter = Control.MOUSE_FILTER_IGNORE

	func _draw() -> void:
		var c := size / 2.0
		var r := minf(size.x, size.y) / 2.0 - 1.5
		draw_arc(c, r, 0.0, TAU, 40, TRACK, 2.5, true)
		if ring_fraction > 0.0:
			draw_arc(c, r, -PI / 2.0, -PI / 2.0 + TAU * ring_fraction, 40, ring_color, 2.5, true)
		draw_circle(c, r * 0.66, color)
		draw_circle(c + Vector2(0, r * 0.33), r * 0.16, Color.WHITE)


var _participant: Participant
var _life: LifeComponent
var _trophy: TrophyController
var _avatar: Avatar
var _state_pill: PanelContainer
var _crown: Label
var _state_text := ""


func _init() -> void:
	add_theme_constant_override("separation", 8)
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func bind(participant: Participant) -> void:
	_participant = participant
	_life = participant.get_life()
	_avatar = Avatar.new()
	_avatar.color = participant.color
	_avatar.ring_color = participant.color
	add_child(_avatar)
	var name_label := CardStyle.label("TÚ", 14)
	name_label.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	add_child(name_label)
	_state_pill = CardStyle.pill("EN JUEGO", CardStyle.OK)
	_state_pill.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	add_child(_state_pill)
	var spacer := Control.new()
	spacer.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	spacer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(spacer)
	_crown = CardStyle.label("♛", 16, CardStyle.GOLD)
	_crown.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	_crown.visible = false
	add_child(_crown)
	participant.color_changed.connect(func(c: Color) -> void:
		_avatar.color = c
		_avatar.queue_redraw())
	Events.trophy_taken.connect(_on_trophy_event.unbind(1))
	Events.trophy_dropped.connect(_on_trophy_event.unbind(2))
	Events.trophy_stolen.connect(_on_trophy_event.unbind(2))
	Events.trophy_delivered.connect(_on_trophy_event.unbind(1))


func set_trophy(trophy: TrophyController) -> void:
	_trophy = trophy
	_on_trophy_event()


func _on_trophy_event() -> void:
	if _trophy == null or _participant == null:
		return
	var carrying := _trophy.get_carrier() == _participant
	if carrying == _crown.visible:
		return
	_crown.visible = carrying
	if carrying:
		_crown.pivot_offset = _crown.size / 2.0
		_crown.scale = Vector2(1.6, 1.6)
		create_tween().tween_property(_crown, "scale", Vector2.ONE, 0.35).set_trans(Tween.TRANS_BACK)


func _process(_delta: float) -> void:
	if _participant == null:
		return
	var text := "EN JUEGO"
	var color := CardStyle.OK
	var ring := _participant.color
	var fraction := 1.0
	if _life != null:
		match _life.get_state():
			LifeComponent.STATE_ELIMINATED:
				text = "ELIMINADO · %d S" % ceili(_life.get_time_left())
				color = CardStyle.DANGER
				ring = CardStyle.DANGER
				fraction = _life.get_time_left() / maxf(_life.respawn_delay, 0.01)
			LifeComponent.STATE_PROTECTED:
				text = "PROTEGIDO"
				color = CardStyle.SHIELD
				ring = CardStyle.SHIELD
				fraction = _life.get_time_left() / maxf(_life.protection_time, 0.01)
	if text != _state_text:
		_state_text = text
		(_state_pill.get_child(0) as Label).text = text
		CardStyle.set_pill_color(_state_pill, color)
	if not is_equal_approx(fraction, _avatar.ring_fraction) or ring != _avatar.ring_color:
		_avatar.ring_fraction = fraction
		_avatar.ring_color = ring
		_avatar.queue_redraw()
