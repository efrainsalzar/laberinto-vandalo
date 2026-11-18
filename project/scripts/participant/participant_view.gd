extends Node2D
## Parte visual provisional de un participante: círculo de su color + punto hacia donde mira.
## Solo dibuja; no tiene lógica. Se reemplazará por un sprite en la pieza 8 (arte).
## Rendimiento (patrón Dirty flag): se redibuja SOLO cuando cambia la dirección o el color, no cada frame.
## Al ser eliminado: se desintegra en píxeles (shaders/dissolve.gdshader) y deja un death_effect (píxeles + marca).
## Al recibir daño (HealthComponent.damaged, §25): destello blanco del cuerpo (0,08 s) y, si es un rival (no lo
## controla una persona), una barrita de salud encima durante unos segundos.

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
	var health: HealthComponent = _participant.get_health()
	if health:
		health.damaged.connect(_on_damaged.unbind(4))


const DEATH_EFFECT := preload("res://scripts/participant/death_effect.gd")
const DISSOLVE := preload("res://shaders/dissolve.gdshader")
const DISSOLVE_SECONDS := 0.5

const FLASH_SECONDS := 0.08
const BAR_SECONDS := 2.5
const BAR_FADE := 0.4

var _blink := false
var _flash := 0.0  # segundos que le quedan al destello blanco
var _bar := 0.0  # segundos que le quedan a la barrita de salud
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


func _on_damaged() -> void:
	_flash = FLASH_SECONDS
	if not (_participant.get_controller() is HumanController):
		_bar = BAR_SECONDS
	queue_redraw()


func _on_respawned() -> void:
	_bar = 0.0
	_flash = 0.0
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


func _process(delta: float) -> void:
	if _flash > 0.0:
		_flash -= delta
		if _flash <= 0.0:
			queue_redraw()
	if _bar > 0.0:
		_bar -= delta
		queue_redraw()  # se desvanece al final (pocos participantes con barra a la vez)
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
	if _flash > 0.0:
		draw_circle(Vector2.ZERO, radius + 1.0, Color(1, 1, 1, 0.9))
	if _bar > 0.0:
		_draw_health_bar(radius)


## Barrita de salud sobre la cabeza (solo rivales, unos segundos después de un golpe).
func _draw_health_bar(radius: float) -> void:
	var health: HealthComponent = _participant.get_health()
	if health == null:
		return
	var alpha := clampf(_bar / BAR_FADE, 0.0, 1.0)
	var width := maxf(18.0, radius * 2.2)
	var rect := Rect2(-width / 2.0, -radius - 8.0, width, 3.0)
	var ratio := health.get_ratio()
	draw_rect(rect.grow(1.0), Color(0.02, 0.01, 0.05, 0.8 * alpha))
	var color := Color(1.0, 0.4, 0.35) if ratio < 0.3 else (Color(1.0, 0.82, 0.3) if ratio < 0.6 else Color(0.45, 0.9, 0.45))
	draw_rect(Rect2(rect.position, Vector2(rect.size.x * ratio, rect.size.y)), Color(color, alpha))
