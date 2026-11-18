class_name FloatingTextLayer
extends CanvasLayer
## Textos flotantes ("+8 VEL") que aparecen en un punto del mundo, suben y se desvanecen.
##
## - Object Pool: crea `pool_size` etiquetas una sola vez y las reutiliza (sin tirones aunque
##   haya muchos textos a la vez). Si todas están ocupadas, recicla la más antigua.
## - Vive en pantalla (no en el mundo): el texto se ve nítido con cualquier zoom y no lo
##   oscurece la niebla. Cada frame se recalcula su posición a partir del punto del mundo.
## - Escucha Events.pickup_collected (Observer), no a los objetos.
## - Números de daño (Events.participant_damaged, §25): blancos sobre rivales, rojos sobre el jugador. Los golpes
##   casi simultáneos al mismo participante (perdigones de la escopeta) se suman en un solo número.
## Ver docs/ARQUITECTURA.md §10.

@export var pool_size := 32
@export var rise_px := 42.0
@export var duration := 1.0

var _pool: Array[Label] = []
var _active: Array[Dictionary] = []  # {label, world, age, tween}
var _created := 0
var _damage_numbers := {}  # id de instancia del participante -> {entry, amount}
const DAMAGE_MERGE_SECONDS := 0.12  # golpes al mismo participante dentro de este tiempo = un solo número
const DAMAGE_PLAYER := Color(1.0, 0.4, 0.35)
const DAMAGE_RIVAL := Color(1.0, 0.97, 0.9)


func _ready() -> void:
	layer = 5
	for i in pool_size:
		_pool.append(_make_label())
	Events.pickup_collected.connect(_on_pickup_collected)
	Events.trophy_taken.connect(func(p: Participant) -> void:
		show_text(p.global_position + Vector2(0, -26), "¡TROFEO!", Color(1.0, 0.82, 0.25), true))
	Events.trophy_dropped.connect(func(_p: Participant, pos: Vector2) -> void:
		show_text(pos, "¡SE CAYÓ EL TROFEO!", Color(1.0, 0.5, 0.35), true))
	Events.participant_damaged.connect(_on_participant_damaged)
	Events.shield_blocked.connect(func(p: Participant, kind: int, reflected: bool) -> void:
		var text := "¡DEVUELTO!" if reflected else ("¡PUM! BURBUJA" if kind == ShieldComponent.SHIELD_BUBBLE else "BLOQUEADO")
		show_text(p.global_position + Vector2(0, -p.body_radius - 4.0), text, Color(0.7, 0.85, 1.0), false, 15))


## Muestra un texto en una posición del mundo. Devuelve su entrada (para poder actualizar el texto).
## font_size > 0 cambia el tamaño normal (19).
func show_text(world_position: Vector2, text: String, color: Color, highlight := false, font_size := 0) -> Dictionary:
	var label: Label
	if _pool.is_empty():
		var oldest: Dictionary = _active.pop_front()
		oldest.tween.kill()
		label = oldest.label
	else:
		label = _pool.pop_back()
	label.text = text
	label.add_theme_color_override("font_color", color)
	label.add_theme_font_size_override("font_size", 26 if highlight else (font_size if font_size > 0 else 19))
	label.reset_size()
	label.pivot_offset = label.size / 2.0
	label.modulate.a = 1.0
	label.scale = Vector2.ONE * (0.4 if highlight else 0.8)
	label.visible = true

	var entry := {"label": label, "world": world_position, "age": 0.0, "tween": null}
	var tween := create_tween().set_parallel(true)
	tween.tween_property(label, "scale", Vector2.ONE, 0.35 if highlight else 0.15) \
		.set_trans(Tween.TRANS_BACK).set_ease(Tween.EASE_OUT)
	tween.tween_property(label, "modulate:a", 0.0, duration * 0.45).set_delay(duration * 0.55)
	tween.chain().tween_callback(_release.bind(entry))
	entry.tween = tween
	_active.append(entry)
	_place(entry)
	return entry


## Cuántas etiquetas se crearon en total (para verificar que el pool reutiliza).
func get_created_count() -> int:
	return _created


func _process(delta: float) -> void:
	for entry in _active:
		entry.age += delta
		_place(entry)


func _place(entry: Dictionary) -> void:
	var t: float = clampf(entry.age / duration, 0.0, 1.0)
	var rise: float = rise_px * (1.0 - pow(1.0 - t, 3.0))  # sube rápido y frena
	var screen: Vector2 = get_viewport().get_canvas_transform() * entry.world
	var label: Label = entry.label
	label.position = screen - label.size / 2.0 - Vector2(0, 18.0 + rise)


func _release(entry: Dictionary) -> void:
	_active.erase(entry)
	entry.label.visible = false
	_pool.append(entry.label)


func _make_label() -> Label:
	var label := Label.new()
	label.visible = false
	label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	label.add_theme_constant_override("outline_size", 6)
	label.add_theme_color_override("font_outline_color", Color(0.02, 0.01, 0.05, 0.9))
	label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(label)
	_created += 1
	return label


func _on_participant_damaged(victim: Participant, _by: Participant, amount: float, _kind: int) -> void:
	if victim == null or amount <= 0.0:
		return
	var key := victim.get_instance_id()
	var previous: Dictionary = _damage_numbers.get(key, {})
	if not previous.is_empty() and previous.entry.age < DAMAGE_MERGE_SECONDS and _active.has(previous.entry):
		previous.amount += amount
		var label: Label = previous.entry.label
		label.text = "-%d" % ceili(previous.amount)
		label.reset_size()
		label.pivot_offset = label.size / 2.0
		return
	var color := DAMAGE_PLAYER if victim.get_controller() is HumanController else DAMAGE_RIVAL
	var offset := Vector2(randf_range(-8.0, 8.0), -victim.body_radius - 4.0)  # que no se tapen entre sí
	var entry := show_text(victim.global_position + offset, "-%d" % ceili(amount), color, false, 16)
	_damage_numbers[key] = {"entry": entry, "amount": amount}


func _on_pickup_collected(type: PickupType, participant: Participant, world_position: Vector2) -> void:
	var info := EffectText.describe_pickup(type)
	# Mejora permanente con el tope ya lleno: avisar "MÁX" en vez de un "+3" que no cambió nada.
	var stats: StatsComponent = participant.get_stats() if participant else null
	if stats and type.effects.size() == 1 and type.effects[0] is StatEffect:
		var effect: StatEffect = type.effects[0]
		if effect.duration < 0 and stats.is_bonus_maxed(effect.stat):
			info.text = "%s MÁX" % ("VEL" if effect.stat == StatsComponent.STAT_SPEED else "VIS")
	show_text(world_position, info.text, info.color, info.highlight)
