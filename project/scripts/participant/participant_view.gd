extends Node2D
## Parte visual provisional de un participante: círculo de su color + punto hacia donde mira.
## Solo dibuja; no tiene lógica. Se reemplazará por un sprite en la pieza 8 (arte).
## Rendimiento (patrón Dirty flag): se redibuja SOLO cuando cambia la dirección o el color, no cada frame.

@onready var _participant: Participant = get_parent()


func _ready() -> void:
	_participant.color_changed.connect(func(_c: Color) -> void: queue_redraw())
	# El cuerpo se ajusta al ancho de pasillo (ver match.gd): la vista lo sigue.
	_participant.body_radius_changed.connect(func(_r: float) -> void: queue_redraw())
	# Eliminado: desaparece. Al volver: parpadea mientras está protegido.
	var life: LifeComponent = _participant.get_life()
	if life:
		life.eliminated.connect(func(_by: Object) -> void: visible = false)
		life.respawned.connect(func() -> void:
			visible = true
			_blink = true)
		life.protection_ended.connect(func() -> void:
			_blink = false
			modulate.a = 1.0)


var _blink := false


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
