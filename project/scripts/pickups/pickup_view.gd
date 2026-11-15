extends Node2D
## Vista provisional de un objeto recolectable: forma del color de su tipo.
## Rendimiento: se dibuja UNA vez (no cada frame); con cientos de objetos en mapas gigantes importa.
## Los objetos raros (poco peso) se dibujan como rombo; los comunes, como círculo.
## Si el tipo tiene `icon`, se dibuja la textura (pieza 8: arte).

const RARE_WEIGHT := 3.0

@onready var _pickup: Pickup = get_parent()


func _ready() -> void:
	# Aparición con rebote (también al reaparecer).
	scale = Vector2.ZERO
	create_tween().tween_property(self, "scale", Vector2.ONE, 0.45).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)



func _draw() -> void:
	var type: PickupType = _pickup.type
	if type == null:
		return
	if type.icon:
		draw_texture(type.icon, -type.icon.get_size() / 2.0)
		return
	var r := type.radius
	var color := type.color
	if type.weight <= RARE_WEIGHT:
		var points := PackedVector2Array([Vector2(0, -r * 1.3), Vector2(r, 0), Vector2(0, r * 1.3), Vector2(-r, 0)])
		draw_colored_polygon(points, color)
		draw_polyline(points + PackedVector2Array([points[0]]), color.lightened(0.5), 1.5)
	else:
		draw_circle(Vector2.ZERO, r, color)
		draw_circle(Vector2.ZERO, r * 0.45, color.lightened(0.6))
