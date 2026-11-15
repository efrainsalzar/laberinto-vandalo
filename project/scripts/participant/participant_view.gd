extends Node2D
## Parte visual provisional de un participante: círculo de su color + punto hacia donde mira.
## Solo dibuja; no tiene lógica. Se reemplazará por un sprite en la pieza 8 (arte).
## Rendimiento (patrón Dirty flag): se redibuja SOLO cuando cambia la dirección o el color, no cada frame.

const RADIUS := 11.0

@onready var _participant: Participant = get_parent()


func _ready() -> void:
	_participant.color_changed.connect(func(_c: Color) -> void: queue_redraw())


var _last_facing := Vector2.ZERO


func _process(_delta: float) -> void:
	var facing := _participant.get_facing()
	if not facing.is_equal_approx(_last_facing):
		_last_facing = facing
		queue_redraw()


func _draw() -> void:
	var color := _participant.color
	draw_circle(Vector2.ZERO, RADIUS, color)
	draw_arc(Vector2.ZERO, RADIUS, 0.0, TAU, 24, color.darkened(0.5), 2.0)
	draw_circle(_participant.get_facing() * (RADIUS - 4.0), 3.0, Color.WHITE)
