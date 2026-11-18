extends Node2D
## Efecto de eliminación en el mundo (solo vista, arte provisional): mientras el cuerpo se desintegra
## (shader en participant_view.gd), salen "píxeles" de su color que se dispersan frenando y se apagan, y queda
## una marca tenue en el piso que se desvanece.
## Se crea en el mundo (no como hijo del participante, que se mueve a su base al reaparecer) y se borra solo.
## Orden de dibujo: z_index 0 y agregado al final del nivel → encima del piso del laberinto; los participantes
## tienen z_index 1, así que pasan por encima de la marca. Ver docs/arquitectura/22-armas-disparo.md.

const BURST_SECONDS := 0.7
const MARK_SECONDS := 4.0
const PIXELS := 26

var _color := Color.WHITE
var _radius := 11.0
var _time := 0.0
var _pixels: Array[Dictionary] = []  # {from, velocity, size, delay}


func setup(color: Color, radius: float) -> void:
	_color = color
	_radius = radius
	for i in PIXELS:
		var dir := Vector2.from_angle(randf() * TAU)
		_pixels.append({
			"from": dir * randf() * radius * 0.8,  # nacen dentro del cuerpo
			"velocity": dir * radius * randf_range(2.5, 6.0),
			"size": maxf(1.5, radius * randf_range(0.14, 0.26)),
			"delay": randf() * 0.25,  # no salen todos juntos: siguen al borde que se desintegra
		})


func _process(delta: float) -> void:
	_time += delta
	if _time >= MARK_SECONDS:
		queue_free()
		return
	queue_redraw()


func _draw() -> void:
	# Marca en el piso: una "x" tenue que se apaga el último segundo.
	var mark_alpha := clampf((MARK_SECONDS - _time) / 1.0, 0.0, 1.0) * 0.4
	var r := _radius * 0.7
	var mark := Color(_color.darkened(0.3), mark_alpha)
	draw_line(Vector2(-r, -r), Vector2(r, r), mark, 3.0)
	draw_line(Vector2(-r, r), Vector2(r, -r), mark, 3.0)
	if _time >= BURST_SECONDS:
		return
	# Píxeles que se dispersan frenando (salida rápida, llegada suave) y se apagan.
	for p in _pixels:
		var t := clampf((_time - p.delay) / (BURST_SECONDS - p.delay), 0.0, 1.0)
		if t <= 0.0:
			continue
		var travel := 1.0 - (1.0 - t) * (1.0 - t)
		var at: Vector2 = p.from + p.velocity * travel * 0.35
		var size: float = p.size * (1.0 - t * 0.6)
		var c := _color.lightened(0.35 * (1.0 - t))
		c.a = 1.0 - t
		draw_rect(Rect2(at - Vector2(size, size) / 2.0, Vector2(size, size)), c)
