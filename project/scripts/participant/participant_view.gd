extends Node2D
## Parte visual provisional de un participante: círculo de su color + punto hacia donde mira.
## Solo dibuja; no tiene lógica. Se reemplazará por un sprite en la pieza 8 (arte).
## Rendimiento (patrón Dirty flag): se redibuja SOLO cuando cambia la dirección o el color, no cada frame.
## Al ser eliminado: se desintegra en píxeles (shaders/dissolve.gdshader) y deja un death_effect (píxeles + marca).

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
const DISSOLVE := preload("res://shaders/dissolve.gdshader")
const DISSOLVE_SECONDS := 0.5

var _blink := false
var _tween: Tween
var _dissolve: ShaderMaterial


## Eliminado: el cuerpo se desintegra en su lugar (shader) mientras salen píxeles de su color (death_effect).
## El participante no se mueve hasta reaparecer, así que la animación ocurre donde cayó.
func _on_eliminated() -> void:
	var effect: Node2D = DEATH_EFFECT.new()
	effect.setup(_participant.color, _participant.body_radius)
	_participant.get_parent().add_child(effect)
	effect.global_position = _participant.global_position
	if _tween:
		_tween.kill()
	if _dissolve == null:
		_dissolve = ShaderMaterial.new()
		_dissolve.shader = DISSOLVE
	_dissolve.set_shader_parameter("edge_color", _participant.color.lightened(0.6))
	_dissolve.set_shader_parameter("pixel_size", maxf(1.5, _participant.body_radius * 0.22))
	_dissolve.set_shader_parameter("body_radius", _participant.body_radius)
	_dissolve.set_shader_parameter("progress", 0.0)
	material = _dissolve
	modulate.a = 1.0
	_blink = false
	_tween = create_tween().set_parallel()
	_tween.tween_method(func(v: float) -> void: _dissolve.set_shader_parameter("progress", v), 0.0, 1.0, DISSOLVE_SECONDS)
	_tween.tween_property(self, "scale", Vector2.ONE * 1.15, DISSOLVE_SECONDS).set_ease(Tween.EASE_OUT)
	_tween.chain().tween_callback(func() -> void:
		visible = false
		material = null
		scale = Vector2.ONE)


func _on_respawned() -> void:
	if _tween:
		_tween.kill()
	material = null
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
