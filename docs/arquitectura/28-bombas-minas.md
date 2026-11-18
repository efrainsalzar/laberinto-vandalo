# §28. Etapa 4.4 — Bombas y minas (objeto usable)

### Problema que resuelve el diseño
Hasta aquí todo objeto hacía efecto al recogerlo. Las bombas y minas se **guardan** y se usan cuando el jugador
quiere, quedan en el piso con un tiempo propio y explotan después. Hacía falta: una acción nueva del participante,
una ranura para guardarlas y un sistema para muchas en el piso sin un nodo por bomba.

### Solución
```
Objeto del mapa (triángulo) ─GiveItemEffect─► ItemComponent (1 ranura, hasta 3)
ParticipantCommand.use_item (Q / clic derecho / RB) ─► ItemComponent.update_use ─► ProjectileSystem.place_explosive
ExplosiveWorld.step ─► ExplosiveEvent ─► ProjectileSystem.explode (= bazuca) ─► escudos ─► salud
```
- **Bomba:** explota a los 2 s. La mecha chispea cada vez más rápido; el último medio segundo se ve el radio.
- **Mina:** se arma en 1 s; armada, explota cuando un rival (nunca su dueño) la toca. El dueño la ve siempre; los
  rivales solo a 1,6 casillas o menos.
- **Explosión:** la misma de la bazuca (radio 2, las paredes protegen, 80 → 30). Sin daño propio (como la bazuca).
- **Una ranura:** mismo objeto = +1 (hasta 3; lleno, el objeto queda en el piso); otro objeto = lo reemplaza.
- **Tope:** 32 en el mapa; al pasarlo desaparece el más viejo. Al perder la vida se pierde lo que llevaba.

| Pieza | Patrón | Por qué |
|---|---|---|
| `ParticipantCommand.use_item` | **Command** | Humano, bot o red usan el objeto igual que disparan. |
| `UsableItem` (.tres) | **Type Object + datos** | Bomba y mina son datos (mecha, armado, radio, daño, color); una nueva = un `.tres`. |
| `GiveItemEffect` | **Strategy + datos** (§26) | Un efecto de objeto más. |
| `ItemComponent` | **Composición** | La mochila como hijo del participante. |
| `ExplosiveWorld` | **Datos en bloque + tope** (como `ProjectileWorld`) | Todas en un arreglo, en orden fijo; lógica en `core` con tests. |
| `ProjectileSystem::explode` | Reutilización | Bazuca, bomba y mina comparten una sola explosión (víctimas, daño, escudos, destello). |

### Archivos
| Archivo | Qué es |
|---|---|
| `src/core/explosives.{h,cpp}` | `ExplosiveSpec`, `Explosive`, `ExplosiveWorld` |
| `tests/test_explosives.cpp` | 5 tests: mecha, armado, dueño y distancia, tope, orden |
| `src/core/participant_command.h` | `use_item` |
| `src/game/usable_item.{h,cpp}` | Objeto usable como datos |
| `src/game/item_component.{h,cpp}` | Mochila (una ranura) |
| `src/game/pickup_effect.*` | `GiveItemEffect` |
| `src/game/projectile_system.*` | `place_explosive`, `explode`, dibujo de bombas y minas, `set_viewer` |
| `project/data/items/*.tres`, `data/pickups/objeto_*.tres`, `data/spawning/usables.tres` | Datos |
| `project/project.godot` | Acción `use_item`: Q, clic derecho, RB del mando |
| `project/scripts/ui/player_card/card_item.gd` | Ranura OBJETO en la tarjeta (●●○ ×2) |
| `project/scripts/pickups/pickup_view.gd` | Triángulo con bomba o mina |

### Decisiones
- Sin daño propio y una sola ranura (recomendación aprobada por el dueño).
- Bots: recogen bombas y minas pero no las usan todavía (anotado en el ROADMAP).
- Teclas de prueba: B = 3 bombas, M = 3 minas.
- Mediciones: `weapons.explosives`.
- Verificado en Godot: compila, 165 tests, la partida carga; prueba temporal: la bomba explota a los 2 s y la mina
  queda esperando; captura de la ranura, la bomba, la mina y los objetos. La prueba en vivo la hace el dueño.
