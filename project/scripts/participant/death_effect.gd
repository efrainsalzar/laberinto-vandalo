extends Node2D
## Efecto de eliminación (solo vista, arte provisional): estallido de fragmentos del color del participante y una
## marca tenue en el piso que se desvanece. Se crea en el mundo (no como hijo del participante, que se mueve a su
## base al reaparecer) y se borra solo. Lo crea participant_view.gd. Ver docs/arquitectura/22-armas-disparo.md.

const BURST_SECONDS := 0.45
const MARK_SECONDS := 4.0
const SHARDS := 10

var _color := Color.WHITE
var _radius := 11.0
var _time := 0.0
var _shards: Array[Vector2] = []  # dirección × velocidad de cada fragmento (px/s)


func setup(color: Color, radius: float) -> void:
	_color = color
	_radius = radius
	for i in SHARDS:
		var angle := TAU * i / SHARDS + randf_range(-0.25, 0.25)
		_shards.append(Vector2.from_angle(angle) * radius * randf_range(5.0, 8.0))
	z_index = -1  # la marca queda debajo de los participantes


func _process(delta: float) -> void:
	_time += delta
	if _time >= MARK_SECONDS:
		queue_free()
		return
	queue_redraw()


func _draw() -> void:
	# Marca en el piso: una "x" tenue que se apaga el último segundo.
	var mark_alpha := clampf((MARK_SECONDS - _time) / 1.0, 0.0, 1.0) * 0.45
	var r := _radius * 0.7
	var mark := Color(_color.darkened(0.3), mark_alpha)
	draw_line(Vector2(-r, -r), Vector2(r, r), mark, 3.0)
	draw_line(Vector2(-r, r), Vector2(r, -r), mark, 3.0)
	if _time >= BURST_SECONDS:
		return
	# Estallido: anillo que se expande + fragmentos que salen frenando.
	var k := _time / BURST_SECONDS
	var fade := 1.0 - k
	draw_arc(Vector2.ZERO, _radius * (1.0 + k * 2.0), 0.0, TAU, 32, Color(_color, fade * 0.8), 2.0)
	var travel := 1.0 - (1.0 - k) * (1.0 - k)  # sale rápido y frena
	for shard in _shards:
		var at := shard * BURST_SECONDS * travel
		draw_circle(at, maxf(1.0, _radius * 0.22 * fade), Color(_color.lightened(0.2), fade))
