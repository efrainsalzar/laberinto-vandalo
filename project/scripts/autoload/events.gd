extends Node
## Event Bus (autoload "Events"): canal central de eventos del juego.
##
## Quien produce un evento lo publica aquí; quien quiera reaccionar (HUD, textos flotantes,
## sonidos, Director de IA...) se conecta aquí. Así ninguno necesita conocer al otro.
## Ver docs/ARQUITECTURA.md §10.
##
## Regla: solo eventos de juego "ya ocurridos" (en pasado). Nada de órdenes ni estado.

## Un participante recogió un objeto. `world_position` es dónde estaba el objeto.
signal pickup_collected(type: PickupType, participant: Participant, world_position: Vector2)

## Apareció un objeto nuevo (reaparición).
signal pickup_spawned(pickup: Pickup)

## Un participante tomó el trofeo.
signal trophy_taken(participant: Participant)

## El trofeo cayó al piso (su portador lo soltó o murió).
signal trophy_dropped(participant: Participant, world_position: Vector2)

## `thief` le quitó el trofeo a `victim` (robo por contacto).
signal trophy_stolen(thief: Participant, victim: Participant)

## `victim` quedó eliminado (hoy: lo atraparon llevando el trofeo). `by` = quién lo atrapó.
signal participant_eliminated(victim: Participant, by: Participant)

## `participant` volvió a su base tras ser eliminado (empieza su protección).
signal participant_respawned(participant: Participant)

## Un participante llevó el trofeo a su base: ganó.
signal trophy_delivered(participant: Participant)
## El laberinto vivo anunció un cambio (casillas que se cerrarán / abrirán en `seconds`).
signal maze_shift_warned(closing: Array, opening: Array, seconds: float)

## Casillas del laberinto que cambiaron (now_wall = true: se cerraron).
signal maze_changed(tiles: Array, now_wall: bool)

## Empezó una fase de la partida (MatchController.PHASE_*). También al empezar (Preparación).
signal match_phase_changed(phase: int, phase_name: String)

## Aviso de tiempo: quedan `seconds_left` segundos de partida.
signal match_time_warning(seconds_left: float)

## Terminó la partida. outcome = MatchController.OUTCOME_VICTORY (winner = quien ganó) u OUTCOME_TIME_UP (winner = null).
signal match_ended(outcome: int, winner: Participant)
