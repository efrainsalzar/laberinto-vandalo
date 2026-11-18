<!-- Sección §21 de docs/ARQUITECTURA.md (índice). -->
# §21. Lenguaje visual de objetos

### Problema que resuelve el diseño
Todos los objetos eran círculos de color. Con armas, portales, objetos de mapa y más actores en camino,
la pantalla se volvería ilegible: hay que saber **qué es** un objeto de un vistazo, antes del arte final (Etapa 7).

### Solución: forma = categoría, símbolo = efecto (datos en `PickupType`)

| Forma (`shape`) | Categoría | Hoy |
|---|---|---|
| Círculo | Mejora de stat | Punto verde (» velocidad), punto amarillo (ojo, visión) |
| Rombo | Power-up temporal | Rapidez (rayo); escudos: burbuja (círculo), frontal (arco), espejo (flecha que rebota) (§27) |
| Cuadrado | Arma | Reservado (pieza de armas) |
| Hexágono | Objeto que cambia el mapa | Reservado (Etapa 2.4) |
| Anillo | Portal | Reservado (Etapa 2.2) |
| Frasco | Poción (curación) | Pequeña y grande, con cruz (§26) |

- `PickupType.shape` y `PickupType.symbol` son enums editables en el inspector (grupo "Aspecto").
- La vista (`scripts/pickups/pickup_view.gd`) dibuja halo + forma con borde oscuro + símbolo, **una sola vez**
  (no cada frame). Si el tipo tiene `icon`, se dibuja la textura en lugar del símbolo: el arte de la Etapa 7
  entra sin tocar reglas.
- Sin archivos de arte nuevos: todo procedimental, nítido a cualquier zoom.

### Archivos
| Archivo | Qué es |
|---|---|
| `src/game/pickup_type.{h,cpp}` | Enums `Shape` y `Symbol` |
| `project/scripts/pickups/pickup_view.gd` | Dibujo por forma y símbolo, parpadeo al caducar |
| `project/data/pickups/*.tres` | Forma y símbolo de cada tipo |

### Decisiones
- Ya no se decide la forma por la rareza (`weight`): la forma es un dato explícito.
- Símbolos nuevos (pistola, escopeta, portal...) se agregan al enum `Symbol` y a `_draw_symbol` cuando llegue su pieza.
