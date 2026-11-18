class_name EffectText
extends RefCounted
## Convierte datos de efectos en texto para la interfaz: "+8 VEL", "×1.5 VEL", "¡RAPIDEZ! ×1.5 VEL".
## Así un objeto nuevo (otro .tres) muestra su texto sin programar nada. Curación: "+25 SAL" (§26).

const STAT_SHORT := {StatsComponent.STAT_SPEED: "VEL", StatsComponent.STAT_VISION: "VIS"}
const HEAL_COLOR := Color(0.4, 1.0, 0.55)
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
	var main_color := type.color
	var found := false
	for effect: PickupEffect in type.effects:
		var effect_color := type.color
		if effect is StatEffect:
			parts.append(format(effect.stat, effect.op, effect.value))
			effect_color = STAT_COLORS.get(effect.stat, type.color)
		elif effect is HealEffect:
			parts.append("+%d SAL" % roundi(effect.amount))
			effect_color = HEAL_COLOR
		else:
			continue  # armas: solo el título
		if not found:
			main_color = effect_color
			found = true
	var text := "  ".join(parts)
	var highlight := not type.popup_title.is_empty()
	if highlight:
		text = ("%s %s" % [type.popup_title, text]).strip_edges()
	var color: Color = type.color if highlight else main_color
	return {"text": text, "color": color, "highlight": highlight}


static func _number(value: float) -> String:
	var v := absf(value)
	return str(int(v)) if is_equal_approx(v, roundf(v)) else str(snappedf(v, 0.01))
