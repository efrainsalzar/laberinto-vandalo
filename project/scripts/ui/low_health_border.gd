extends Control
## Borde rojo de la pantalla cuando la salud del jugador baja de 30 (§25). Late más rápido cuanto menos queda.
## Solo dibuja; escucha HealthComponent (health_changed) del participante de esta pantalla.
## Arte provisional: franjas que se desvanecen hacia adentro (sin shader).

const LOW := 0.3
const BANDS := 10
const BAND_PX := 6.0
const COLOR := Color(1.0, 0.12, 0.1)

var _health: HealthComponent
var _strength := 0.0  # 0 = no se ve; 1 = salud casi en 0
var _alpha := 0.0  # lo que se dibuja (sube y baja suave)


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)


func bind(participant: Participant) -> void:
	_health = participant.get_health()
	if _health:
		_health.health_changed.connect(_on_health_changed)
		_on_health_changed(_health.get_health(), _health.get_max_health())


func _on_health_changed(current: float, max_value: float) -> void:
	var ratio := current / maxf(max_value, 0.001)
	# En 0 la vida ya se perdió (vuelve a su base): no hace falta avisar.
	_strength = 0.0 if ratio >= LOW or ratio <= 0.0 else 1.0 - ratio / LOW


func _process(delta: float) -> void:
	var target := 0.0
	if _strength > 0.0:
		var speed := lerpf(4.0, 9.0, _strength)  # latido
		var pulse := 0.5 + 0.5 * sin(Time.get_ticks_msec() / 1000.0 * speed)
		target = lerpf(0.35, 0.75, _strength) * lerpf(0.65, 1.0, pulse)
	_alpha = move_toward(_alpha, target, delta * 3.0)
	visible = _alpha > 0.001
	if visible:
		queue_redraw()


func _draw() -> void:
	var s := size
	for i in BANDS:
		var a := _alpha * pow(1.0 - float(i) / BANDS, 2.0) * 0.5
		var c := Color(COLOR, a)
		var o := i * BAND_PX
		draw_rect(Rect2(o, o, s.x - o * 2.0, BAND_PX), c)  # arriba
		draw_rect(Rect2(o, s.y - o - BAND_PX, s.x - o * 2.0, BAND_PX), c)  # abajo
		draw_rect(Rect2(o, o + BAND_PX, BAND_PX, s.y - o * 2.0 - BAND_PX * 2.0), c)  # izquierda
		draw_rect(Rect2(s.x - o - BAND_PX, o + BAND_PX, BAND_PX, s.y - o * 2.0 - BAND_PX * 2.0), c)  # derecha
