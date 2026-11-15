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

## Un participante llevó el trofeo a su base: ganó.
signal trophy_delivered(participant: Participant)