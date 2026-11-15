extends Node2D
## Vista provisional de una zona de resguardo (base): cuadro del color de su dueño con borde que late.
## Tiene una luz tenue para poder reconocer tu base en la oscuridad. Solo dibuja.
## `size_px` = tamaño exacto de la zona (el cuadro de pasillo de la base): se ajusta al ancho de pasillo.

var color := Color(0.35, 0.75, 1.0):
	set(value):
		color = value
		if is_node_ready():
			$Glow.color = value
		queue_redraw()

var size_px := 48.0:
	set(value):
		size_px = value
		queue_redraw()

var _time := 0.0


func _ready() -> void:
	$Glow.color = color


func _process(delta: float) -> void:
	_time += delta
	queue_redraw()


func _draw() -> void:
	var half := size_px / 2.0
	var rect := Rect2(-half, -half, size_px, size_px)
	draw_rect(rect, Color(color, 0.16))
	var pulse := 0.55 + 0.35 * sin(_time * 2.0)
	draw_rect(rect.grow(-1.0), Color(color, pulse), false, 2.0)
	# Esquinas marcadas, como una zona de aterrizaje.
	var c := minf(8.0, size_px * 0.3)
	var h := half - 1.5  # las líneas miden 3 px: se meten 1,5 px para no pisar las paredes
	for corner in [Vector2(-h, -h), Vector2(h, -h), Vector2(h, h), Vector2(-h, h)]:
		var sx := -signf(corner.x)
		var sy := -signf(corner.y)
		draw_line(corner, corner + Vector2(sx * c, 0), color, 3.0)
		draw_line(corner, corner + Vector2(0, sy * c), color, 3.0)
