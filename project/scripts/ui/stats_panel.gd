extends PanelContainer
## Panel de interfaz que muestra las stats de un participante y sus efectos activos.
## Observer: solo escucha las señales del StatsComponent; no modifica nada.
## Uso: stats_panel.bind(participant)

const STAT_NAMES := {StatsComponent.STAT_SPEED: "Velocidad", StatsComponent.STAT_VISION: "Visión"}

@onready var _name: Label = $Rows/Name
@onready var _speed: Label = $Rows/Speed
@onready var _vision: Label = $Rows/Vision
@onready var _effects: Label = $Rows/Effects

var _stats: StatsComponent


func bind(participant: Participant) -> void:
	_stats = participant.get_stats()
	_name.text = participant.display_name
	_name.add_theme_color_override("font_color", participant.color)
	if _stats == null:
		_speed.text = "(sin StatsComponent)"
		return
	_stats.stat_changed.connect(_on_stat_changed)
	_on_stat_changed(StatsComponent.STAT_SPEED, _stats.get_speed())
	_on_stat_changed(StatsComponent.STAT_VISION, _stats.get_vision())


func _process(_delta: float) -> void:
	# Los segundos restantes cambian cada frame; el resto se actualiza por señales.
	if _stats == null:
		return
	var lines: PackedStringArray = []
	for m: Dictionary in _stats.get_modifiers():
		var amount: String = ("+%s" % m.value) if m.op == StatsComponent.OP_ADD else ("×%s" % m.value)
		var time: String = "" if m.remaining < 0.0 else " (%.1fs)" % m.remaining
		lines.append("%s %s · %s%s" % [STAT_NAMES[m.stat], amount, m.source, time])
	_effects.text = "\n".join(lines)
	_effects.visible = not lines.is_empty()


func _on_stat_changed(stat: int, value: float) -> void:
	match stat:
		StatsComponent.STAT_SPEED:
			_speed.text = "Velocidad: %d px/s" % roundi(value)
		StatsComponent.STAT_VISION:
			_vision.text = "Visión: %.1f casillas" % value
