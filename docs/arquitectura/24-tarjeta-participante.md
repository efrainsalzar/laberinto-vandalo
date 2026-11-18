<!-- Sección §24 de docs/ARQUITECTURA.md (índice). -->
# §24. Tarjeta del participante (HUD)

### Problema que resuelve el diseño
La información del jugador estaba repartida: VEL, VIS, efectos y "llevas el trofeo" en la barra de arriba, y las
armas en un panel abajo al centro. Con la salud y las vidas (Etapa 4) la barra de arriba no daba para más. El dueño
pidió un panel propio, pequeño y moderno, que crezca sin rehacerlo.

### Solución (v0.0.33): tarjeta del boceto arriba a la izquierda, sin barras
```
╭──────────────────────────────╮   ⟨ 6:42 · ASALTO ⟩          60 FPS (opcional)
│ (◉) TÚ                       │    ♛ lo lleva ROJO
│     [EN JUEGO] [♛ TROFEO]    │   (avisos debajo)
│ SAL ████████████░░   100     │  salud (el anillo del avatar también la muestra)
│ VEL ▬▬▬▬▬▬░░░░   120  ×0,85  │  stats: sólido = permanente, rayado = temporal
│ VIS ▬▬▬▬░░░░░░   5.0         │
│ (◔ RAPIDEZ 4 s)              │  efectos temporales, solo mientras duran
│ ──────────────────────────── │
│ [BASE PISTOLA ∞][ESPECIAL …] │  dos ranuras; la activa con el color del arma
╰──────────────────────────────╯                               F1 · controles
```
- **Tarjeta** flotante (296 px) arriba a la izquierda, con el diseño del boceto que eligió el dueño (v0.0.33).
  Si un participante pasa por detrás, se vuelve semitransparente.
- **Arriba al centro**, una pastilla con el reloj y la fase (en el último minuto se pone roja y crece) y, debajo, la
  **línea del trofeo**: en la bóveda / en el piso / lo lleva ROJO / ¡LO LLEVAS TÚ!
- **FPS** solo si está activado en Opciones (arriba a la derecha). **Objetos recogidos** e **info del mapa** salen
  de la pantalla: van en los resultados y al pie de la ayuda (F1).
- Historia: v0.0.29 tarjeta grande arriba con barra superior → v0.0.31 compacta abajo a la izquierda → v0.0.33 vuelve
  el diseño del boceto, arriba a la izquierda y sin barra superior (pedido del dueño).

| Pieza | Patrón | Por qué |
|---|---|---|
| `player_card.gd` | **Composite** (contenedor + lista `SECTIONS`) | Agregar información = agregar una sección. Salud y vidas (Etapa 4) entran así, sin tocar las demás. |
| `card_header.gd`, `card_stats.gd`, `card_effects.gd`, `card_weapons.gd` | **Observer** | Cada sección escucha solo su fuente (`LifeComponent` + eventos del trofeo, `StatsComponent`, `WeaponComponent`). Nunca modifican el juego. |
| `card_style.gd` | Estilo común | Colores de la paleta del juego y piezas (píldora, etiqueta, divisor) para que todas las secciones se vean igual. |

**Detalles.** El anillo del avatar: tu salud con tu color (rojo bajo 30) mientras estás en juego, celeste que se vacía (protegido), rojo que cuenta
la espera (eliminado). Mientras estás eliminado, stats, efectos y armas se apagan. Las barras de stats usan el
`StatsProfile` (base + tope de mejoras); "MÁX" en dorado al llegar al tope. Munición: segmentos si son pocos tiros
(≤ 12: escopeta, bazuca), barra si es batería (láser); en rojo y latiendo con un cuarto o menos.

### Archivos
| Archivo | Qué es |
|---|---|
| `project/scripts/ui/player_card/*.gd` | La tarjeta y sus secciones |
| `project/scripts/ui/hud.gd`, `project/scenes/ui/hud.tscn` | Crea la tarjeta; pastilla del reloj + línea del trofeo; FPS opcional; sin barra superior |
| (borrado) `project/scripts/ui/weapon_panel.gd` | Reemplazado por la sección de armas |

### Decisiones
- Solo interfaz (GDScript): no hizo falta C++ (el `StatsProfile` ya expone base y tope).
- Cabecera, stats y armas se consultan por señales; efectos y estado de vida se consultan cada frame (pocos datos).
- Verificado en Godot: carga sin errores; capturas en estado normal y eliminado (con la desintegración).
