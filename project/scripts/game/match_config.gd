class_name MatchConfig
extends Resource
## Configuración de una partida elegida en el menú: perfil del mapa y semilla.
## La pantalla de configuración la arma; la partida la lee de App.match_config.

@export var profile: MazeProfile
## 0 = aleatoria en cada partida. Distinta de 0 = mismo mapa siempre (útil para compartir un mapa).
@export var seed := 0


static func create_default() -> MatchConfig:
	var config := MatchConfig.new()
	config.profile = load("res://data/maps/normal.tres").duplicate()
	return config
