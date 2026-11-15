class_name ObjectiveIndicator
extends Control
## Indicador de objetivo: flecha en el borde de la pantalla que apunta al objetivo actual
## (el trofeo, o tu base si lo llevas), con su nombre y distancia. Si el objetivo está en
## pantalla, se muestra un marcador pequeño encima de él. Solo dibuja; el HUD le dice qué mirar.

const EDGE_MARGIN := 34.0
const TOP_RESERVED := 46.0  # barra superior del HUD

var target_position := Vector2.ZERO  # en el mundo
var target_name := ""
var target_color := Color.WHITE
var distance_tiles := 0.0
var active := false

var _time := 0.0
var _font: Font


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_preset(Control.PRESET_FULL_RECT)
	_font = get_theme_default_font()


func _process(delta: float) -> void:
	_time += delta
	queue_redraw()


func _draw() -> void:
	if not active:
		return
	var screen: Vector2 = get_viewport().get_canvas_transform() * target_position
	var area := Rect2(Vector2(EDGE_MARGIN, TOP_RESERVED + EDGE_MARGIN),
		size - Vector2(EDGE_MARGIN * 2.0, TOP_RESERVED + EDGE_MARGIN * 2.0))
	var label := "%s · %d" % [target_name, roundi(distance_tiles)]
	if area.has_point(screen):
		# En pantalla: chevron que flota sobre el objetivo.
		var bob := 4.0 * sin(_time * 5.0)
		var tip := screen + Vector2(0, -30 + bob)
		_draw_arrow(tip, Vector2.DOWN, 10.0)
		return
	# Fuera de pantalla: flecha en el borde, apuntando hacia el objetivo.
	var center := area.get_center()
	var dir := (screen - center).normalized()
	var edge := _ray_to_rect(center, dir, area)
	var pulse := 1.0 + 0.12 * sin(_time * 6.0)
	_draw_arrow(edge, dir, 14.0 * pulse)
	var text_size := _font.get_string_size(label, HORIZONTAL_ALIGNMENT_CENTER, -1, 15)
	var text_pos := edge - dir * 34.0 - Vector2(text_size.x / 2.0, -5.0)
	text_pos.x = clampf(text_pos.x, 4.0, size.x - text_size.x - 4.0)
	draw_string_outline(_font, text_pos, label, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, 5, Color(0.02, 0.01, 0.05, 0.9))
	draw_string(_font, text_pos, label, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, target_color)


func _draw_arrow(tip: Vector2, dir: Vector2, length: float) -> void:
	var side := Vector2(-dir.y, dir.x) * length * 0.6
	var back := tip - dir * length
	var points := PackedVector2Array([tip, back + side, back - side])
	draw_colored_polygon(points, target_color)
	draw_polyline(points + PackedVector2Array([tip]), Color(0.02, 0.01, 0.05, 0.9), 2.0)


## Punto donde un rayo desde el centro sale del rectángulo.
func _ray_to_rect(origin: Vector2, dir: Vector2, rect: Rect2) -> Vector2:
	var t := INF
	if dir.x > 0.0:
		t = minf(t, (rect.end.x - origin.x) / dir.x)
	elif dir.x < 0.0:
		t = minf(t, (rect.position.x - origin.x) / dir.x)
	if dir.y > 0.0:
		t = minf(t, (rect.end.y - origin.y) / dir.y)
	elif dir.y < 0.0:
		t = minf(t, (rect.position.y - origin.y) / dir.y)
	return origin + dir * t
