<!-- Sección §14 de docs/ARQUITECTURA.md (índice). -->
# §14. Interfaz: efectos globales, descripciones flotantes y mapas grandes

### Efectos de interfaz globales (`UiFx`, autoload)
- Se engancha a **cada** botón, barra y campo que aparece en cualquier pantalla (señal `node_added` del árbol):
  ningún menú repite código. Es un **Decorator en tiempo de ejecución** (agrega comportamiento sin tocar las clases) + **Observer**.
- **Hover = selección** (como en consolas): pasar el mouse da el foco; mouse, teclado y mando se comportan igual y
  nunca hay dos opciones resaltadas. El resaltado escala ×1,035, ilumina y dibuja una barra de acento dorada.
- **Clic**: pequeño rebote. **Descripciones flotantes**: `UiFx.hint(control, "texto")`; el globo aparece junto al
  control tras 0,35 s (a la derecha, o a la izquierda/debajo si no cabe), se desliza y se desvanece al salir.
  Reemplaza los textos fijos que llenaban los menús.

### Mapas más grandes
- `MazeProfile.size_scale` (Normal ×1, Grande ×1,5, Enorme ×2, Gigante ×3) se aplica a los niveles; en
  Personalizado el tamaño se elige con las barras (hasta 160×110 celdas). Los objetos escalan con el tamaño.
- **Rendimiento medido** (Difícil, sin VSync): Normal 105×73 casillas ≈ 600 FPS; **Gigante 313×217 casillas con
  468 objetos ≈ 440-560 FPS**. Generar el Gigante tarda 1-2 ms.
- **Optimización:** los objetos se dibujan una sola vez (antes se redibujaban cada frame por un "pulso" decorativo:
  con 468 objetos bajaba a ~70 FPS). Regla: **ninguna vista que haya en cantidad se redibuja cada frame.**
