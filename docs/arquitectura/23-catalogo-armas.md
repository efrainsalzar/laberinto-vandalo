<!-- Sección §23 de docs/ARQUITECTURA.md (índice). -->
# §23. Catálogo de armas: escopeta, láser y bazuca (+ armas en el mapa y HUD)

### Problema que resuelve el diseño
El juego necesita muchas armas. Si cada arma fuera una clase, 20 armas serían 20 clases con código casi repetido.
Además, por ahora el dueño quiere armas **básicas** (todas matan al impactar); su comportamiento propio se
especifica más adelante. Hacía falta una base que escale sin reescribir.

### Solución: cada arma es una combinación de piezas de datos
| Pieza (`WeaponSpec` / `WeaponType`) | Opciones | Uso |
|---|---|---|
| **Cómo dispara** (`fire_mode` → `FireBehavior`, Strategy) | `Proyectil` · `Rayo sostenido` | pistola, escopeta y bazuca vuelan; el láser es un rayo continuo |
| **Perdigones** (`pellets`, `spread_deg`) | 1..24 en un cono, repartidos parejos (sin azar) | escopeta: 7 en 30° |
| **Impacto** (`explosion_radius`) | 0 = directo · >0 = explosión | bazuca: radio 2 casillas |
| **Munición** (`ammo`) o **batería** (`energy`, segundos) | 0 = infinita | pistola infinita; escopeta 8 y bazuca 4 tiros; láser 4 s |
| **Alcance** (`range_from_vision` + factor) | visión × factor, o fijo | escopeta 0,75 × visión |

Un arma nueva que combine estas piezas = **un .tres, sin código** (ametralladora, francotirador, granada...).
Un comportamiento realmente nuevo (rebote, veneno, cargar el disparo) = una opción nueva en una de estas piezas.

**Inventario (decisión del dueño): pistola fija + 1 ranura especial.** `WeaponComponent.equip_special()` la equipa
sola y reemplaza a la anterior. Se vuelve a la pistola cuando la especial se queda sin munición o al ser eliminado.

```mermaid
classDiagram
    direction LR
    class WeaponComponent { +weapon (base) ; +equip_special(w) ; +clear_special() ; +get_active_weapon() ; signal weapon_changed(w) ; signal ammo_changed(n) }
    class WeaponType { <<datos .tres>> fire_mode, pellets, spread_deg, explosion_radius, ammo }
    class ProjectileSystem { +spawn() ; +fire_beam() ; signal participant_hit ; signal exploded }
    class spread_directions { <<core>> }
    class trace_beam { <<core>> }
    class explosion_victims { <<core>> }
    WeaponComponent o-- "base + especial" WeaponType
    WeaponComponent ..> spread_directions : perdigones
    WeaponComponent ..> ProjectileSystem
    ProjectileSystem ..> trace_beam : láser
    ProjectileSystem ..> explosion_victims : bazuca
```

| Pieza | Patrón | Por qué |
|---|---|---|
| `WeaponType` con `fire_mode`, perdigones, explosión, munición | **Type Object** + armas por composición de datos | Escala a un catálogo grande sin clases nuevas. |
| `FireBehavior` (`ProjectileFire`, `HeldBeamFire`) | **Strategy** + Factory (`FireBehavior::create(mode)`) | Cada manera de disparar es una clase que recibe el gatillo (apretado, mantenido, soltado). Un arma de carga, ráfaga o cuerpo a cuerpo = una clase nueva; `WeaponComponent` no cambia. |
| Ranura especial en `WeaponComponent` | **Composición** + State por arma | Cada arma lleva su propio `WeaponState` (cadencia y munición independientes). |
| `trace_beam`, `explosion_victims`, `spread_directions` | Funciones puras en `core` | Se prueban sin Godot; reutilizan el DDA de la visión. |

**Láser (v0.0.28): rayo sostenido.** Mientras se mantiene el clic hay **un solo rayo continuo** (antes eran rayos
sueltos cada 0,15 s que parpadeaban). El arma lo renueva cada frame con `ProjectileSystem.hold_beam` (sigue la
puntería y le pega en el acto al primer blanco que toca); si un frame no se renueva (gatillo suelto, sin batería,
tirador eliminado) se apaga. Gasta 4 s de batería solo mientras dispara.
**Gatillo:** `Participant` pasa cada frame si el gatillo está abajo; `WeaponComponent` calcula `Trigger`
(`Pressed`, `Held`, `Released`, `Idle`) y se lo da al comportamiento del arma activa.
**Bots con láser:** el error de puntería se sortea cada 0,25 s (`BotGunner.error_hold`), así el rayo barre con
suavidad en vez de temblar en cada frame. **Bazuca:** cohete a 22 casillas/s que explota al chocar o al llegar a su alcance; la explosión le pega
a todos los que estén en el radio y tengan línea de visión con el centro (las paredes protegen). No hay daño propio
(decisión del dueño, por ahora). El punto de choque contra una pared se retrocede 0,05 casillas para que la explosión
no nazca dentro de la pared.

### Armas en el mapa y HUD (v0.0.26)
- **Objeto-arma = un `PickupType` con `weapon`** (`data/pickups/arma_*.tres`): forma **cuadrado** (reservada para
  armas desde §21) y símbolo propio: abanico (escopeta), haz (láser), estrella (bazuca). Al recogerlo,
  `Pickup.collect` llama a `WeaponComponent.equip_special`. Sin código por arma: **agregar un arma al mapa =
  un `WeaponType` + un `PickupType` + sumarlo a la categoría**.
- **Categoría "armas"** (`data/spawning/armas.tres`): cupo propio (5 % del presupuesto, entre 1 y 8), reposición cada
  35 s, caducan a los 30 s. Rareza por `weight`: escopeta 3, láser 2, bazuca 1. Reutiliza `PickupSpawner` (§20).
- **Bots:** recogen armas como cualquier objeto que ven (acción "recoger objeto") y disparan con la que tengan.
  Buscarlas a propósito queda para la pieza de bots.
- **HUD** (v0.0.26; desde v0.0.29 es la sección de armas de la tarjeta del participante, §24) (`scripts/ui/weapon_panel.gd`, abajo al centro): ranura PISTOLA (∞) y ranura ESPECIAL con nombre, munición
  (en rojo con 2 o menos) y barra del color del arma; la activa se resalta y la especial late al recogerla. Solo vista:
  escucha `weapon_changed` y `ammo_changed` (Observer).

### Rendimiento
| Medición | Qué |
|---|---|
| `weapons.projectiles` | Balas + efectos (rayos y explosiones que se apagan) |
| `weapons.beam` | Un rayo de láser (al disparar) |

Escopeta: 7 balas por disparo; con 8 participantes con escopeta son ~70 balas por segundo, muy debajo del tope (256).

### Archivos
| Archivo | Qué es |
|---|---|
| `src/core/weapon.{h,cpp}` | `FireMode`, perdigones, explosión, munición total; `spread_directions` |
| `src/core/projectiles.{h,cpp}` | `trace_beam`, `explosion_victims`; balas con explosión |
| `tests/test_weapons.cpp` | +5 tests: cono parejo, munición que se gasta, rayo (primer blanco, tirador, paredes), explosión (radio, borde, paredes, tirador), choque sobre suelo |
| `src/game/weapon_type.*`, `weapon_component.*`, `projectile_system.*` | Datos, ranura especial, rayos sostenidos y explosiones |
| `src/game/fire_behavior.{h,cpp}` | Comportamientos de disparo (Strategy): `ProjectileFire`, `HeldBeamFire` |
| `project/data/weapons/{pistol,shotgun,laser,bazooka}.tres` | El catálogo |
| `project/scripts/game/match.gd` | Teclas de prueba 4/5/6 (equipar) y 7 (volver a la pistola) |
| `src/game/pickup_type.*`, `pickup.cpp` | `PickupType.weapon` y símbolos de armas; equipar al recoger |
| `project/data/pickups/arma_*.tres`, `data/spawning/armas.tres` | Objetos-arma y su categoría |
| `project/scripts/pickups/pickup_view.gd` | Dibujo de los símbolos de armas |
| `project/scripts/ui/weapon_panel.gd`, `hud.gd` | Panel de armas del HUD |

### Decisiones
- Todas las armas matan al impactar (dueño). La especificación propia de cada arma queda para después.
- Al ser eliminado se pierde el arma especial.
- Verificado en Godot: compila, 135 tests, la partida carga sin errores. Prueba en vivo: el dueño.
