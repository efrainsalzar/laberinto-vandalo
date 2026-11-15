extends Node2D
## Vista provisional del trofeo: copa dorada que flota en su pedestal, o sobre la cabeza del portador.
## Tiene su propia luz: se ve de lejos aunque esté en la oscuridad (VISION §6: "visible para todos").
## Solo dibuja; las reglas viven en TrophyController / core.

var carried := false:
	set(value):
		carried = value
		queue_redraw()

const GOLD := Color(1.0, 0.82, 0.25)
var _time := 0.0


func _process(delta: float) -> void:
	_time += delta
	queue_redraw()


func _draw() -> void:
	var lift := -22.0 if carried else -4.0 + 2.0 * sin(_time * 2.5)
	var o := Vector2(0, lift)
	if not carried:
		draw_rect(Rect2(-9, 6, 18, 5), Color(0.45, 0.4, 0.55))  # pedestal
	# Copa: cuerpo, asas, pie y base.
	draw_colored_polygon(PackedVector2Array([o + Vector2(-8, -10), o + Vector2(8, -10), o + Vector2(5, -1), o + Vector2(-5, -1)]), GOLD)
	draw_arc(o + Vector2(-8, -6), 3.5, PI * 0.5, PI * 1.5, 8, GOLD, 2.0)
	draw_arc(o + Vector2(8, -6), 3.5, -PI * 0.5, PI * 0.5, 8, GOLD, 2.0)
	draw_rect(Rect2(o + Vector2(-1.5, -1), Vector2(3, 4)), GOLD)
	draw_rect(Rect2(o + Vector2(-5, 3), Vector2(10, 2.5)), GOLD.darkened(0.2))
	draw_circle(o + Vector2(-3, -7), 1.5, Color(1, 1, 0.9, 0.9))  # brillo
