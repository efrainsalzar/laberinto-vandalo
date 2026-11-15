class_name EffectText
extends RefCounted
## Convierte datos de efectos en texto para la interfaz: "+8 VEL", "×1.5 VEL", "¡RAPIDEZ! ×1.5 VEL".
## Así un objeto nuevo (otro .tres) muestra su texto sin programar nada.

const STAT_SHORT := {StatsComponent.STAT_SPEED: "VEL", StatsComponent.STAT_VISION: "VIS"}
const STAT_COLORS := {
	StatsComponent.STAT_SPEED: Color(0.35, 1.0, 0.6),
	StatsComponent.STAT_VISION: Color(1.0, 0.88, 0.35),
}


## Texto corto de un efecto/modificador. `op` y `value` como en StatsComponent.
static func format(stat: int, op: int, value: float) -> String:
	var amount := _number(value)
	var name: String = STAT_SHORT.get(stat, "?")
	if op == StatsComponent.OP_MULTIPLY:
		return "×%s %s" % [amount, name]
	return "%s%s %s" % ["+" if value >= 0.0 else "", amount, name]


## Texto, color y si es "destacado" para el texto flotante de un tipo de objeto.
static func describe_pickup(type: PickupType) -> Dictionary:
	var parts: PackedStringArray = []
	var main_stat := -1
	for effect: PickupEffect in type.effects:
		parts.append(format(effect.stat, effect.op, effect.value))
		if main_stat < 0:
			main_stat = effect.stat
	var text := "  ".join(parts)
	var highlight := not type.popup_title.is_empty()
	if highlight:
		text = "%s %s" % [type.popup_title, text]
	var color: Color = type.color if highlight else STAT_COLORS.get(main_stat, type.color)
	return {"text": text, "color": color, "highlight": highlight}


static func _number(value: float) -> String:
	var v := absf(value)
	return str(int(v)) if is_equal_approx(v, roundf(v)) else str(snappedf(v, 0.01))
