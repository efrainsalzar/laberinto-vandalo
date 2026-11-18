extends PanelContainer
## Tarjeta del participante (HUD, arriba a la izquierda): quién eres, tu estado, stats, efectos y armas.
##
## Patrón Composite: la tarjeta es un contenedor con una lista de SECCIONES. Cada sección es independiente,
## escucha solo su fuente (Observer) y nunca modifica el juego:
##   cabecera → LifeComponent + eventos del trofeo · stats → StatsComponent · efectos → modificadores temporales
##   · armas → WeaponComponent.
## Agregar información nueva (p. ej. salud y vidas en la Etapa 4) = una sección nueva en SECTIONS.
## API de una sección: bind(participant) y, si la necesita, set_trophy(trophy).
## Ver docs/arquitectura/24-tarjeta-participante.md.

const CardStyle := preload("res://scripts/ui/player_card/card_style.gd")
const SECTIONS: Array[Script] = [
	preload("res://scripts/ui/player_card/card_header.gd"),
	preload("res://scripts/ui/player_card/card_stats.gd"),
	preload("res://scripts/ui/player_card/card_effects.gd"),
	preload("res://scripts/ui/player_card/card_weapons.gd"),
]
## Secciones que se apagan mientras el participante está eliminado (la cabecera sigue viva: cuenta la espera).
const DIM_WHEN_DOWN := ["card_stats", "card_effects", "card_weapons"]

var _rows: VBoxContainer
var _sections: Array[Control] = []
var _participant: Participant
var _down := false


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	custom_minimum_size.x = CardStyle.WIDTH
	var style := StyleBoxFlat.new()
	style.bg_color = CardStyle.PANEL
	style.border_color = CardStyle.LINE
	style.set_border_width_all(1)
	style.set_corner_radius_all(12)
	style.shadow_color = Color(0, 0, 0, 0.4)
	style.shadow_size = 8
	style.content_margin_left = 14
	style.content_margin_right = 14
	style.content_margin_top = 12
	style.content_margin_bottom = 12
	add_theme_stylebox_override("panel", style)
	_rows = VBoxContainer.new()
	_rows.add_theme_constant_override("separation", 10)
	_rows.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_rows)
	# Arriba a la izquierda, debajo de la barra superior.
	set_anchors_preset(Control.PRESET_TOP_LEFT)
	position = Vector2(12, 46)


## Conecta la tarjeta al participante de esta pantalla (crea las secciones).
func bind(participant: Participant) -> void:
	_participant = participant
	for script in SECTIONS:
		var section: Control = script.new()
		section.name = script.resource_path.get_file().get_basename()
		if section.name == "card_weapons":
			_rows.add_child(CardStyle.divider())
		_rows.add_child(section)
		_sections.append(section)
		section.bind(participant)


## El trofeo llega después (lo crea la partida): se lo pasa a las secciones que lo usan.
func set_trophy(trophy: TrophyController) -> void:
	for section in _sections:
		if section.has_method("set_trophy"):
			section.set_trophy(trophy)


func _process(_delta: float) -> void:
	if _participant == null:
		return
	var down := not _participant.is_active()
	if down == _down:
		return
	_down = down
	for section in _sections:
		if section.name in DIM_WHEN_DOWN:
			create_tween().tween_property(section, "modulate", Color(0.6, 0.58, 0.68, 0.55) if down else Color.WHITE, 0.25)
