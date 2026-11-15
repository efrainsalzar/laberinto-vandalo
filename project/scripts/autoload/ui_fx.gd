extends CanvasLayer
## Efectos de interfaz globales (autoload "UiFx").
##
## Se engancha automáticamente a cada botón, barra o campo que aparece en CUALQUIER pantalla
## (señal `node_added` del árbol): no hay que repetir código en cada menú.
## - Hover/foco: el control crece un poco, se ilumina y muestra una barra de acento a la izquierda.
## - Clic: pequeño rebote.
## - Descripción flotante: si el control tiene una descripción (UiFx.hint(control, "...")), aparece
##   un globo junto a él tras una breve pausa y se desvanece al salir. Funciona con mouse y teclado.
## Patrón: Decorator en tiempo de ejecución (agrega comportamiento sin tocar las clases) + Observer.
## Ver docs/ARQUITECTURA.md §14.

const HOVER_SCALE := Vector2(1.035, 1.035)
const HOVER_TINT := Color(1.14, 1.12, 1.2)
const HINT_DELAY := 0.35
const HINT_MAX_WIDTH := 300.0
const ACCENT := Color(1.0, 0.82, 0.25)
const META_HINT := "ui_hint"
const META_DONE := "ui_fx"

var _bubble: PanelContainer
var _bubble_label: Label
var _bubble_owner: Control
var _bubble_tween: Tween
var _pending_owner: Control
var _pending_time := 0.0


## Asigna una descripción corta a un control (se muestra en el globo flotante).
func hint(control: Control, text: String) -> void:
	control.set_meta(META_HINT, text)


func _ready() -> void:
	layer = 90
	process_mode = Node.PROCESS_MODE_ALWAYS
	_build_bubble()
	get_tree().node_added.connect(_on_node_added)


func _process(delta: float) -> void:
	if _pending_owner == null:
		return
	if not is_instance_valid(_pending_owner) or not _pending_owner.is_visible_in_tree():
		_pending_owner = null
		return
	_pending_time += delta
	if _pending_time >= HINT_DELAY:
		_show_bubble(_pending_owner)
		_pending_owner = null


func _on_node_added(node: Node) -> void:
	if node is BaseButton or node is Slider or node is LineEdit:
		_attach.call_deferred(node)


func _attach(control: Control) -> void:
	if not is_instance_valid(control) or control.has_meta(META_DONE):
		return
	control.set_meta(META_DONE, true)
	control.mouse_entered.connect(_on_mouse_enter.bind(control))
	control.mouse_exited.connect(_on_mouse_exit.bind(control))
	control.focus_entered.connect(_on_focus_enter.bind(control))
	control.focus_exited.connect(_on_focus_exit.bind(control))
	if control is BaseButton:
		control.button_down.connect(_on_press.bind(control))
		control.draw.connect(_draw_accent.bind(control))


# --- Hover / foco ---------------------------------------------------------------------------
# Como en los menús de consola: pasar el mouse por encima SELECCIONA (da el foco), así nunca hay
# dos opciones resaltadas y mouse/teclado/mando se comportan igual.

func _on_mouse_enter(control: Control) -> void:
	var selectable := (control is BaseButton or control is Slider) and control.focus_mode != Control.FOCUS_NONE
	if selectable and not (control is BaseButton and control.disabled):
		control.grab_focus()
	else:
		_highlight(control, true)
	_queue_hint(control)


func _on_mouse_exit(control: Control) -> void:
	_cancel_hint(control)
	if not control.has_focus():
		_highlight(control, false)


func _on_focus_enter(control: Control) -> void:
	_highlight(control, true)
	_queue_hint(control)


func _on_focus_exit(control: Control) -> void:
	_highlight(control, false)
	_cancel_hint(control)


func _highlight(control: Control, on: bool) -> void:
	if not (control is BaseButton):
		return
	if on and control.disabled:
		return
	_animate(control, HOVER_SCALE if on else Vector2.ONE, HOVER_TINT if on else Color.WHITE, 0.12 if on else 0.16)
	control.set_meta("ui_hover", on)
	control.queue_redraw()


func _queue_hint(control: Control) -> void:
	if control.has_meta(META_HINT) and _bubble_owner != control:
		_pending_owner = control
		_pending_time = 0.0


func _cancel_hint(control: Control) -> void:
	if _pending_owner == control:
		_pending_owner = null
	if _bubble_owner == control:
		_hide_bubble()

func _on_press(control: Control) -> void:
	control.pivot_offset = control.size / 2.0
	var t := control.create_tween()
	t.tween_property(control, "scale", Vector2(0.96, 0.96), 0.05)
	t.tween_property(control, "scale", HOVER_SCALE, 0.12).set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)


func _animate(control: Control, target_scale: Vector2, tint: Color, time: float) -> void:
	control.pivot_offset = control.size / 2.0
	if control.has_meta("ui_tween"):
		var old: Tween = control.get_meta("ui_tween")
		if old and old.is_valid():
			old.kill()
	var t := control.create_tween().set_parallel(true).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	t.tween_property(control, "scale", target_scale, time)
	t.tween_property(control, "self_modulate", tint, time)
	control.set_meta("ui_tween", t)


## Barra de acento a la izquierda del botón resaltado.
func _draw_accent(control: Control) -> void:
	if control.get_meta("ui_hover", false) and not (control is CheckButton):
		control.draw_rect(Rect2(3, 6, 3, control.size.y - 12), ACCENT)


# --- Globo de descripción -------------------------------------------------------------------

func _build_bubble() -> void:
	_bubble = PanelContainer.new()
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.07, 0.06, 0.12, 0.97)
	style.border_color = Color(ACCENT, 0.8)
	style.set_border_width_all(1)
	style.border_width_left = 3
	style.set_corner_radius_all(6)
	style.content_margin_left = 12
	style.content_margin_right = 12
	style.content_margin_top = 8
	style.content_margin_bottom = 8
	style.shadow_color = Color(0, 0, 0, 0.5)
	style.shadow_size = 8
	_bubble.add_theme_stylebox_override("panel", style)
	_bubble.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_bubble_label = Label.new()
	_bubble_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_bubble_label.custom_minimum_size.x = 120
	_bubble_label.add_theme_font_size_override("font_size", 15)
	_bubble_label.add_theme_color_override("font_color", Color(0.9, 0.88, 0.98))
	_bubble.add_child(_bubble_label)
	_bubble.visible = false
	add_child(_bubble)


func _show_bubble(control: Control) -> void:
	_bubble_owner = control
	_bubble_label.text = control.get_meta(META_HINT)
	# Ancho según el texto, con un máximo.
	var font := _bubble_label.get_theme_font("font")
	var natural := font.get_string_size(_bubble_label.text, HORIZONTAL_ALIGNMENT_LEFT, -1, 15).x
	_bubble_label.custom_minimum_size.x = minf(natural + 2.0, HINT_MAX_WIDTH)
	_bubble.size = Vector2.ZERO
	_bubble.reset_size()
	_bubble.visible = true
	# Posición: a la derecha del control; si no cabe, a la izquierda; si tampoco, debajo.
	var rect := control.get_global_rect()
	var view := get_viewport().get_visible_rect().size
	var pos := Vector2(rect.end.x + 12.0, rect.position.y + (rect.size.y - _bubble.size.y) / 2.0)
	var slide := Vector2(-8, 0)
	if pos.x + _bubble.size.x > view.x - 8.0:
		pos.x = rect.position.x - _bubble.size.x - 12.0
		slide = Vector2(8, 0)
	if pos.x < 8.0:
		pos = Vector2(rect.position.x, rect.end.y + 8.0)
		slide = Vector2(0, -6)
	pos = pos.clamp(Vector2(8, 8), view - _bubble.size - Vector2(8, 8))
	if _bubble_tween and _bubble_tween.is_valid():
		_bubble_tween.kill()
	_bubble.position = pos + slide
	_bubble.modulate.a = 0.0
	_bubble_tween = create_tween().set_parallel(true).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	_bubble_tween.tween_property(_bubble, "position", pos, 0.16)
	_bubble_tween.tween_property(_bubble, "modulate:a", 1.0, 0.16)


func _hide_bubble() -> void:
	_bubble_owner = null
	if _bubble_tween and _bubble_tween.is_valid():
		_bubble_tween.kill()
	_bubble_tween = create_tween()
	_bubble_tween.tween_property(_bubble, "modulate:a", 0.0, 0.12)
	_bubble_tween.tween_callback(func() -> void: _bubble.visible = false)
