extends VBoxContainer
## Tarjeta del participante (HUD): flotante y compacta, abajo a la izquierda, pegada a la esquina.
## Arriba van los efectos temporales como fichitas (solo mientras duran); debajo, la tarjeta:
## cabecera (avatar, estado, trofeo) · stats (VEL, VIS) · arma activa.
## Si un participante pasa por detrás, se vuelve semitransparente para no tapar el juego.
##
## Patrón Composite: la tarjeta es un contenedor con una lista de SECCIONES. Cada sección es independiente,
## escucha solo su fuente (Observer) y nunca modifica el juego:
##   cabecera → LifeComponent + eventos del trofeo · stats → StatsComponent · efectos → modificadores temporales
##   · armas → WeaponComponent.
## Agregar información nueva (p. ej. salud y vidas en la Etapa 4) = una sección nueva en SECTIONS.
## API de una sección: bind(participant) y, si la necesita, set_trophy(trophy).
## Ver docs/arquitectura/24-tarjeta-participante.md.

const CardStyle := preload("res://scripts/ui/player_card/card_style.gd")
const EFFECTS := preload("res://scripts/ui/player_card/card_effects.gd")
const SECTIONS: Array[Script] = [
	preload("res://scripts/ui/player_card/card_header.gd"),
	preload("res://scripts/ui/player_card/card_stats.gd"),
	preload("res://scripts/ui/player_card/card_weapons.gd"),
]
## Secciones que se apagan mientras el participante está eliminado (la cabecera sigue viva: cuenta la espera).
const DIM_WHEN_DOWN := ["card_stats", "card_weapons", "card_effects"]
const MARGIN := 14
const FADED_ALPHA := 0.3  # con algo detrás

var _body: PanelContainer
var _rows: VBoxContainer
var _sections: Array[Control] = []
var _participant: Participant
var _down := false


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_theme_constant_override("separation", 6)
	alignment = BoxContainer.ALIGNMENT_END
	_body = PanelContainer.new()
	_body.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_body.custom_minimum_size.x = CardStyle.WIDTH
	var style := StyleBoxFlat.new()
	style.bg_color = CardStyle.PANEL
	style.border_color = CardStyle.LINE
	style.set_border_width_all(1)
	style.set_corner_radius_all(10)
	style.shadow_color = Color(0, 0, 0, 0.35)
	style.shadow_size = 6
	style.content_margin_left = 10
	style.content_margin_right = 10
	style.content_margin_top = 8
	style.content_margin_bottom = 9
	_body.add_theme_stylebox_override("panel", style)
	_rows = VBoxContainer.new()
	_rows.add_theme_constant_override("separation", 6)
	_rows.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_body.add_child(_rows)


## Conecta la tarjeta al participante de esta pantalla (crea las secciones).
func bind(participant: Participant) -> void:
	_participant = participant
	var effects: Control = EFFECTS.new()  # encima de la tarjeta, fuera del panel
	effects.name = "card_effects"
	add_child(effects)
	_sections.append(effects)
	effects.bind(participant)
	add_child(_body)
	for script in SECTIONS:
		var section: Control = script.new()
		section.name = script.resource_path.get_file().get_basename()
		_rows.add_child(section)
		_sections.append(section)
		section.bind(participant)
	# Abajo a la izquierda, pegada a la esquina; crece hacia arriba cuando aparecen efectos.
	grow_vertical = Control.GROW_DIRECTION_BEGIN
	set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_LEFT, Control.PRESET_MODE_MINSIZE, MARGIN)


## El trofeo llega después (lo crea la partida): se lo pasa a las secciones que lo usan.
func set_trophy(trophy: TrophyController) -> void:
	for section in _sections:
		if section.has_method("set_trophy"):
			section.set_trophy(trophy)


func _process(delta: float) -> void:
	if _participant == null:
		return
	_update_dim()
	# Se aparta: si un participante está detrás de la tarjeta, se vuelve semitransparente.
	var area := _body.get_global_rect().grow(10.0)
	var covered := false
	for node in get_tree().get_nodes_in_group("participants"):
		var p := node as Participant
		if p != null and p.is_active() and area.has_point(p.get_global_transform_with_canvas().origin):
			covered = true
			break
	var target := FADED_ALPHA if covered else 1.0
	modulate.a = move_toward(modulate.a, target, delta * 4.0)


func _update_dim() -> void:
	var down := not _participant.is_active()
	if down == _down:
		return
	_down = down
	for section in _sections:
		if section.name in DIM_WHEN_DOWN:
			create_tween().tween_property(section, "modulate", Color(0.6, 0.58, 0.68, 0.55) if down else Color.WHITE, 0.25)
