class_name MatchConfig
extends Resource
## Configuración de una partida elegida en el menú: perfil del mapa y semilla.
## La pantalla de configuración la arma; la partida la lee de App.match_config.

@export var profile: MazeProfile
## 0 = aleatoria en cada partida. Distinta de 0 = mismo mapa siempre (útil para compartir un mapa).
@export var seed := 0
## Laberinto vivo: 0 apagado, 1 lento, 2 normal, 3 caótico.
@export_enum("Apagado", "Lento", "Normal", "Caótico") var live_maze := 2

## Ritmo del laberinto vivo por nivel: [segundos entre cambios, pares (abrir+cerrar) por cambio].
const LIVE_MAZE_LEVELS := [[0.0, 0], [25.0, 1], [15.0, 2], [7.0, 4]]


static func create_default() -> MatchConfig:
	var config := MatchConfig.new()
	config.profile = load("res://data/maps/normal.tres").duplicate()
	return config
