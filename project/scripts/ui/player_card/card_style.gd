extends RefCounted
## Colores, tamaños y piezas comunes de la tarjeta del participante (para que todas las secciones se vean igual).
## Paleta del juego (ui/theme.tres): paneles violeta oscuro, bordes lavanda, dorado del trofeo.

const WIDTH := 296.0
const PANEL := Color(0.05, 0.04, 0.09, 0.86)
const LINE := Color(0.45, 0.4, 0.65, 0.45)
const LINE_SOFT := Color(0.45, 0.4, 0.65, 0.22)
const INK := Color(0.91, 0.89, 0.97)
const INK_DIM := Color(0.62, 0.59, 0.72)
const INK_FAINT := Color(0.42, 0.39, 0.52)
const GOLD := Color(1.0, 0.82, 0.25)
const OK := Color(0.45, 0.9, 0.45)
const SHIELD := Color(0.56, 0.82, 1.0)
const DANGER := Color(1.0, 0.4, 0.35)
const TRACK := Color(1, 1, 1, 0.08)


static func label(text: String, size: int, color := INK) -> Label:
	var l := Label.new()
	l.text = text
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", color)
	l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return l


## Etiqueta en mayúsculas pequeña y espaciada (VEL, BASE, ESPECIAL...).
static func caption(text: String, color := INK_FAINT) -> Label:
	var l := label(text, 11, color)
	l.uppercase = true
	return l


## Píldora de estado: texto en mayúsculas con borde del mismo color.
static func pill(text: String, color: Color) -> PanelContainer:
	var panel := PanelContainer.new()
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var l := label(text, 11, color)
	l.uppercase = true
	panel.add_child(l)
	set_pill_color(panel, color)
	return panel


static func set_pill_color(panel: PanelContainer, color: Color, fill := 0.0) -> void:
	var style := StyleBoxFlat.new()
	style.bg_color = Color(color, fill)
	style.border_color = color
	style.set_border_width_all(1)
	style.set_corner_radius_all(99)
	style.content_margin_left = 8
	style.content_margin_right = 8
	style.content_margin_top = 0
	style.content_margin_bottom = 1
	panel.add_theme_stylebox_override("panel", style)
	(panel.get_child(0) as Label).add_theme_color_override("font_color", color)


static func divider() -> ColorRect:
	var line := ColorRect.new()
	line.color = LINE_SOFT
	line.custom_minimum_size = Vector2(0, 1)
	line.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return line


## "×1,5" (coma decimal, como el resto de la interfaz).
static func multiplier_text(value: float) -> String:
	var text := str(snappedf(value, 0.01))
	return "×" + text.replace(".", ",")
