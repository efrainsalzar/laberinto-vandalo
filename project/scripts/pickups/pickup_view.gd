extends Node2D
## Vista de un objeto recolectable con el lenguaje visual del juego (ARQUITECTURA §21):
## - La FORMA dice la categoría: círculo = mejora de stat, rombo = power-up, cuadrado = arma,
##   hexágono = objeto que cambia el mapa, anillo = portal, frasco = poción.
## - El SÍMBOLO de adentro dice qué hace: » velocidad, ojo visión, rayo rapidez; armas: abanico (escopeta),
##   haz (láser), estrella (bazuca); cruz = cura.
## - Halo suave del color del tipo y borde oscuro para que se lea sobre cualquier suelo.
## Rendimiento: se dibuja UNA vez (no cada frame); con cientos de objetos en mapas gigantes importa.
## Si el tipo tiene `icon`, se dibuja la textura en lugar del símbolo (arte de la Etapa 7).

const OUTLINE := Color(0.05, 0.05, 0.08, 0.9)

@onready var _pickup: Pickup = get_parent()


func _ready() -> void:
	# Aparición con rebote (también al reaparecer).
	scale = Vector2.ZERO
	create_tween().tween_property(self, "scale", Vector2.ONE, 0.45).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	_pickup.expiring.connect(_on_expiring)


## Va a desaparecer pronto: parpadea hasta que el spawner lo retire.
func _on_expiring() -> void:
	var tween := create_tween().set_loops()
	tween.tween_property(self, "modulate:a", 0.25, 0.25)
	tween.tween_property(self, "modulate:a", 1.0, 0.25)


func _draw() -> void:
	var type: PickupType = _pickup.type
	if type == null:
		return
	var r := type.radius
	var color := type.color
	# Halo: dos círculos translúcidos, más grandes que la forma.
	draw_circle(Vector2.ZERO, r * 2.0, Color(color, 0.10))
	draw_circle(Vector2.ZERO, r * 1.5, Color(color, 0.18))
	_draw_shape(type.shape, r, color)
	if type.icon:
		var size := Vector2(r, r) * 1.4
		draw_texture_rect(type.icon, Rect2(-size / 2.0, size), false)
	else:
		_draw_symbol(type.symbol, r * 0.62, color.darkened(0.65))


func _draw_shape(shape: int, r: float, color: Color) -> void:
	match shape:
		PickupType.SHAPE_DIAMOND:
			_draw_polygon(_regular(4, r * 1.25, 0.0), color)
		PickupType.SHAPE_SQUARE:
			_draw_polygon(_regular(4, r * 1.25, PI / 4.0), color)
		PickupType.SHAPE_HEXAGON:
			_draw_polygon(_regular(6, r * 1.15, PI / 6.0), color)
		PickupType.SHAPE_FLASK:  # frasco: cuerpo redondo + cuello
			var neck := Rect2(-r * 0.32, -r * 1.45, r * 0.64, r * 0.75)
			draw_rect(neck.grow(1.2), OUTLINE)
			draw_circle(Vector2(0, r * 0.15), r + 1.2, OUTLINE)
			draw_rect(neck, color.lightened(0.25))
			draw_rect(Rect2(-r * 0.42, -r * 1.55, r * 0.84, r * 0.22), color.darkened(0.35))  # tapón
			draw_circle(Vector2(0, r * 0.15), r, color)
			draw_circle(Vector2(-r * 0.32, -r * 0.15), r * 0.26, color.lightened(0.5))  # brillo
		PickupType.SHAPE_RING:
			draw_arc(Vector2.ZERO, r, 0.0, TAU, 32, OUTLINE, r * 0.55 + 2.0, true)
			draw_arc(Vector2.ZERO, r, 0.0, TAU, 32, color, r * 0.55, true)
		_:
			draw_circle(Vector2.ZERO, r + 1.2, OUTLINE)
			draw_circle(Vector2.ZERO, r, color)
			draw_circle(Vector2(-r * 0.3, -r * 0.3), r * 0.3, color.lightened(0.5))  # brillo


func _draw_symbol(symbol: int, s: float, ink: Color) -> void:
	var w := maxf(1.2, s * 0.32)
	match symbol:
		PickupType.SYMBOL_SPEED:  # »
			for dx: float in [-s * 0.45, s * 0.25]:
				draw_polyline(PackedVector2Array([Vector2(dx - s * 0.3, -s * 0.6), Vector2(dx + s * 0.3, 0), Vector2(dx - s * 0.3, s * 0.6)]), ink, w, true)
		PickupType.SYMBOL_VISION:  # ojo
			var lid := PackedVector2Array()
			for i in 13:
				var a := PI * i / 12.0
				lid.append(Vector2(-cos(a) * s, -sin(a) * s * 0.55))
			for i in range(1, 12):
				var a := PI * i / 12.0
				lid.append(Vector2(cos(a) * s, sin(a) * s * 0.55))
			lid.append(lid[0])
			draw_polyline(lid, ink, w * 0.8, true)
			draw_circle(Vector2.ZERO, s * 0.32, ink)
		PickupType.SYMBOL_SPREAD:  # abanico: tres líneas que se abren
			for a: float in [-0.45, 0.0, 0.45]:
				var d := Vector2.from_angle(a - PI / 2.0)
				draw_line(Vector2(0, s * 0.7), Vector2(0, s * 0.7) + d * s * 1.5, ink, w * 0.8, true)
		PickupType.SYMBOL_BEAM:  # haz: línea diagonal con destello en la punta
			draw_line(Vector2(-s * 0.8, s * 0.8), Vector2(s * 0.5, -s * 0.5), ink, w, true)
			draw_circle(Vector2(s * 0.55, -s * 0.55), s * 0.3, ink)
		PickupType.SYMBOL_BLAST:  # estrella de explosión
			var star := PackedVector2Array()
			for i in 16:
				var rr := s if i % 2 == 0 else s * 0.45
				star.append(Vector2.from_angle(TAU * i / 16.0) * rr)
			draw_colored_polygon(star, ink)
		PickupType.SYMBOL_CROSS:  # cruz +: cura (un poco más abajo, en el cuerpo del frasco)
			var c := Vector2(0, s * 0.25)
			draw_line(c + Vector2(-s * 0.6, 0), c + Vector2(s * 0.6, 0), ink, w * 1.2)
			draw_line(c + Vector2(0, -s * 0.6), c + Vector2(0, s * 0.6), ink, w * 1.2)
		PickupType.SYMBOL_BOLT:  # rayo
			draw_colored_polygon(PackedVector2Array([
				Vector2(s * 0.2, -s), Vector2(-s * 0.55, s * 0.12), Vector2(-s * 0.02, s * 0.12),
				Vector2(-s * 0.2, s), Vector2(s * 0.55, -s * 0.12), Vector2(s * 0.02, -s * 0.12)]), ink)


func _draw_polygon(points: PackedVector2Array, color: Color) -> void:
	var closed := points + PackedVector2Array([points[0]])
	draw_polyline(closed, OUTLINE, 3.0, true)
	draw_colored_polygon(points, color)
	draw_polyline(closed, color.lightened(0.45), 1.2, true)


func _regular(sides: int, radius: float, rotation_offset: float) -> PackedVector2Array:
	var points := PackedVector2Array()
	for i in sides:
		var a := rotation_offset + TAU * i / sides - PI / 2.0
		points.append(Vector2(cos(a), sin(a)) * radius)
	return points
