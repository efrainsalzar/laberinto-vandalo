extends Node2D
## Parte visual provisional de un participante: círculo de su color + punto hacia donde mira.
## Solo dibuja; no tiene lógica. Se reemplazará por un sprite en la pieza 8 (arte).
## Rendimiento (patrón Dirty flag): se redibuja SOLO cuando cambia la dirección o el color, no cada frame.
## Al ser eliminado: se infla y se desvanece, y deja un death_effect (estallido + marca) en el mundo.

@onready var _participant: Participant = get_parent()


func _ready() -> void:
	_participant.color_changed.connect(func(_c: Color) -> void: queue_redraw())
	# El cuerpo se ajusta al ancho de pasillo (ver match.gd): la vista lo sigue.
	_participant.body_radius_changed.connect(func(_r: float) -> void: queue_redraw())
	# Eliminado: estalla y desaparece (queda una marca en el piso). Al volver: aparece y parpadea mientras está protegido.
	var life: LifeComponent = _participant.get_life()
	if life:
		life.eliminated.connect(_on_eliminated.unbind(1))
		life.respawned.connect(_on_respawned)
		life.protection_ended.connect(func() -> void:
			_blink = false
			modulate.a = 1.0)


const DEATH_EFFECT := preload("res://scripts/participant/death_effect.gd")

var _blink := false
var _tween: Tween


func _on_eliminated() -> void:
	var effect: Node2D = DEATH_EFFECT.new()
	effect.setup(_participant.color, _participant.body_radius)
	_participant.get_parent().add_child(effect)
	effect.global_position = _participant.global_position
	if _tween:
		_tween.kill()
	_tween = create_tween().set_parallel()
	_tween.tween_property(self, "scale", Vector2.ONE * 1.6, 0.15).set_ease(Tween.EASE_OUT)
	_tween.tween_property(self, "modulate:a", 0.0, 0.15)
	_tween.chain().tween_callback(func() -> void:
		visible = false
		scale = Vector2.ONE
		modulate.a = 1.0)


func _on_respawned() -> void:
	if _tween:
		_tween.kill()
	visible = true
	modulate.a = 1.0
	scale = Vector2.ONE * 0.2
	_tween = create_tween()
	_tween.tween_property(self, "scale", Vector2.ONE, 0.3).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	_blink = true


var _last_facing := Vector2.ZERO


func _process(_delta: float) -> void:
	if _blink:
		modulate.a = 0.35 if int(Time.get_ticks_msec() / 120) % 2 == 0 else 1.0
	var facing := _participant.get_facing()
	if not facing.is_equal_approx(_last_facing):
		_last_facing = facing
		queue_redraw()


func _draw() -> void:
	var color := _participant.color
	var radius := _participant.body_radius
	draw_circle(Vector2.ZERO, radius, color)
	draw_arc(Vector2.ZERO, radius, 0.0, TAU, 24, color.darkened(0.5), 2.0)
	var eye := maxf(1.5, radius * 0.27)
	draw_circle(_participant.get_facing() * (radius - eye - 1.0), eye, Color.WHITE)
