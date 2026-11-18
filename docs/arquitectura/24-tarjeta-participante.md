<!-- Sección §24 de docs/ARQUITECTURA.md (índice). -->
# §24. Tarjeta del participante (HUD)

### Problema que resuelve el diseño
La información del jugador estaba repartida: VEL, VIS, efectos y "llevas el trofeo" en la barra de arriba, y las
armas en un panel abajo al centro. Con la salud y las vidas (Etapa 4) la barra de arriba no daba para más. El dueño
pidió un panel propio, pequeño y moderno, que crezca sin rehacerlo.

### Solución: una tarjeta con secciones (arriba a la izquierda)
```
┌───────────────────────────────┐
│ (◉) TÚ                         │  cabecera: avatar con anillo de estado + píldoras
│      [EN JUEGO] [♛ TROFEO]     │
│ VEL ▬▬▬▬▬▬▬▬▬▬░░░  120  ×0,85  │  stats: barra hasta el tope (sólido = permanente, rayado = temporal)
│ VIS ▬▬▬▬▬▬▬░░░░░░  5.0          │
│ (◔ ×1.5 VEL 4 s)               │  efectos: anillo que se vacía (solo si hay)
│ ───────────────────────────── │
│ [BASE PISTOLA ∞][ESPECIAL LÁSER 2,3 s ▬▬▬] │  armas: la activa con el color del arma
└───────────────────────────────┘
```
La barra superior queda solo para la partida (reloj, objetos, FPS). Maqueta aprobada por el dueño.

| Pieza | Patrón | Por qué |
|---|---|---|
| `player_card.gd` | **Composite** (contenedor + lista `SECTIONS`) | Agregar información = agregar una sección. Salud y vidas (Etapa 4) entran así, sin tocar las demás. |
| `card_header.gd`, `card_stats.gd`, `card_effects.gd`, `card_weapons.gd` | **Observer** | Cada sección escucha solo su fuente (`LifeComponent` + eventos del trofeo, `StatsComponent`, `WeaponComponent`). Nunca modifican el juego. |
| `card_style.gd` | Estilo común | Colores de la paleta del juego y piezas (píldora, etiqueta, divisor) para que todas las secciones se vean igual. |

**Detalles.** El anillo del avatar: lleno con tu color (en juego), celeste que se vacía (protegido), rojo que cuenta
la espera (eliminado). Mientras estás eliminado, stats, efectos y armas se apagan. Las barras de stats usan el
`StatsProfile` (base + tope de mejoras); "MÁX" en dorado al llegar al tope. Munición: segmentos si son pocos tiros
(≤ 12: escopeta, bazuca), barra si es batería (láser); en rojo y latiendo con un cuarto o menos.

### Archivos
| Archivo | Qué es |
|---|---|
| `project/scripts/ui/player_card/*.gd` | La tarjeta y sus secciones |
| `project/scripts/ui/hud.gd`, `project/scenes/ui/hud.tscn` | Crea la tarjeta; la barra de arriba queda con FPS y OBJ |
| (borrado) `project/scripts/ui/weapon_panel.gd` | Reemplazado por la sección de armas |

### Decisiones
- Solo interfaz (GDScript): no hizo falta C++ (el `StatsProfile` ya expone base y tope).
- Cabecera, stats y armas se consultan por señales; efectos y estado de vida se consultan cada frame (pocos datos).
- Verificado en Godot: carga sin errores; captura con la tarjeta en estado normal. Estados en partida: prueba del dueño.
