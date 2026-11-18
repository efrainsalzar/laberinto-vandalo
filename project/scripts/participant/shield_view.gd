extends Node2D
## Vista de los escudos de un participante (§27). Solo dibuja; escucha ShieldComponent.
## - Burbuja: anillo celeste alrededor del cuerpo.
## - Frontal: arco azul de ±60° hacia donde apunta.
## - Espejo: anillo plateado con destellos que giran.
## En los últimos 2 s parpadea (se está acabando). Al parar un golpe, el escudo destella.
## Rendimiento: solo procesa y redibuja mientras hay escudos.

const BUBBLE := Color(0.55, 0.85, 1.0)
const FRONTAL := Color(0.45, 0.62, 1.0)
const MIRROR := Color(0.88, 0.9, 0.97)
const ENDING_SECONDS := 2.0
const HIT_FLASH_SECONDS := 0.15

@onready var _participant: Participant = get_parent()
var _shields: ShieldComponent
var _hit_flash := {}  # kind -> segundos que le quedan al destello


func _ready() -> void:
	_shields = _participant.get_shields()
	set_process(false)
	if _shields == null:
		return
	_shields.shield_added.connect(func(_k: int) -> void: set_process(true))
	_shields.shield_ended.connect(func(_k: int) -> void: queue_redraw())
	_shields.blocked.connect(func(kind: int, _reflected: bool) -> void: _hit_flash[kind] = HIT_FLASH_SECONDS)


func _process(delta: float) -> void:
	for kind in _hit_flash.keys():
		_hit_flash[kind] -= delta
		if _hit_flash[kind] <= 0.0:
			_hit_flash.erase(kind)
	queue_redraw()
	if _shields.is_empty() and _hit_flash.is_empty():
		set_process(false)


func _draw() -> void:
	if _shields == null or not _participant.is_active():
		return
	var r := _participant.body_radius
	var t := Time.get_ticks_msec() / 1000.0
	for s: Dictionary in _shields.get_shields():
		var kind: int = s.kind
		var alpha := 1.0
		if s.remaining < ENDING_SECONDS and int(t * 8.0) % 2 == 0:
			alpha = 0.35  # se está acabando
		var flash: bool = _hit_flash.has(kind)
		match kind:
			ShieldComponent.SHIELD_BUBBLE:
				var rr := r + 5.0
				draw_circle(Vector2.ZERO, rr, Color(BUBBLE, 0.12 * alpha))
				draw_arc(Vector2.ZERO, rr, 0.0, TAU, 40, Color(Color.WHITE if flash else BUBBLE, 0.85 * alpha), 2.0, true)
			ShieldComponent.SHIELD_FRONTAL:
				var aim := _participant.get_aim().angle()
				var half := deg_to_rad(_shields.frontal_half_angle)
				var rr := r + 8.0
				var color := Color(Color.WHITE if flash else FRONTAL, 0.9 * alpha)
				draw_arc(Vector2.ZERO, rr, aim - half, aim + half, 24, Color(FRONTAL, 0.25 * alpha), 7.0, true)
				draw_arc(Vector2.ZERO, rr, aim - half, aim + half, 24, color, 3.0, true)
			ShieldComponent.SHIELD_MIRROR:
				var rr := r + 3.0
				draw_arc(Vector2.ZERO, rr, 0.0, TAU, 40, Color(Color.WHITE if flash else MIRROR, 0.7 * alpha), 1.5, true)
				for i in 3:  # destellos que giran
					var a := t * 2.5 + TAU * i / 3.0
					draw_arc(Vector2.ZERO, rr, a, a + 0.5, 8, Color(Color.WHITE, 0.95 * alpha), 3.0, true)
