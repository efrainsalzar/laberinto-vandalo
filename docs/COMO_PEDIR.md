# Cómo pedirle tareas al agente (y gastar pocos tokens)

> Guía para el dueño del proyecto. Cuanto más concreto el pedido, menos tiene que leer y adivinar el agente,
> y menos tokens se gastan. El agente no necesita leer esto.

## Las 5 costumbres que más ahorran

1. **Una conversación por pieza.** Al terminar el commit de una pieza, `/clear` y empezar de cero.
   Todo lo importante ya queda en los documentos y en git. Si una conversación se alarga a mitad de
   una pieza, `/compact`.
2. **Decir qué, dónde y cómo se nota.** Qué debe pasar, en qué sistema y cómo vas a comprobar que funciona.
3. **Un problema por mensaje.** Si probando encuentras 3 fallos, mejor una lista numerada en un mensaje que
   tres conversaciones mezcladas; y si son grandes, de a uno.
4. **Contar lo que viste, no solo "no funciona".** "El bot se queda girando en la base cuando el trofeo está en el
   piso" ahorra al agente toda la búsqueda.
5. **Aprobar el diseño corto.** Si el diseño propuesto te sirve, basta con "ok" o "dale". Si quieres cambios,
   di solo cuáles.

## Plantillas

### Pieza nueva del ROADMAP
```
/nuevo-modulo <nombre de la pieza>
Qué quiero: <comportamiento en 1–3 frases>.
Cómo lo voy a probar: <qué hago en la partida y qué debería ver>.
Fuera de alcance: <lo que NO quieres todavía, p. ej. arte, sonido>.
```
Ejemplo:
```
/nuevo-modulo disparar
Qué quiero: con Espacio el participante lanza un proyectil en la dirección en que mira; si toca a otro, lo aturde 1 s.
Los bots también disparan (una BotAction).
Cómo lo voy a probar: disparo a un bot y veo que se detiene; un bot me dispara cuando llevo el trofeo.
Fuera de alcance: daño y vidas (eso es la Etapa 4 completa), arte del proyectil (un punto basta).
```

### Arreglar un comportamiento
```
fix: <qué sistema> (si lo sabes: archivo o clase)
Pasos: 1) ... 2) ... 3) ...
Pasa esto: <lo que viste>.
Debería pasar: <lo esperado>.
```
Ejemplo:
```
fix: bots / BotBrain
Pasos: partida Normal, 3 bots Difícil; tomo el trofeo y lo suelto cerca de mi base.
Pasa esto: los 3 bots se quedan quietos junto a la bóveda.
Debería pasar: al menos uno va por el trofeo caído.
```

### Ajustar números (balance)
Casi todo está en datos `.tres` (`project/data/`). Pide el cambio directo:
```
Ajuste: portador a ×0,9 en vez de ×0,85 y reaparición a 2 s. Solo datos, sin código.
```

### Preguntar sin cambiar nada
```
Pregunta (no cambies código): ¿cómo decide un bot cuándo emboscar? Respuesta corta.
```

## Qué evitar (cuesta muchos tokens)

- "Revisa todo el proyecto y mejóralo" o "busca bugs en general": obliga a leerlo todo.
- Mezclar en un pedido una pieza nueva, un arreglo y un cambio de interfaz.
- Pedir arte, colores o sonido ahora: la presentación es la **Etapa 7**. Hasta entonces, puntos y colores
  provisionales, para que el comportamiento se pueda cambiar sin rehacer gráficos.

## Orden recomendado en esta etapa

Mientras el juego es de puntos y colores, el foco es **que el comportamiento sea el correcto y que nada se rompa**:

1. **Pieza por pieza** según el ROADMAP (acciones del participante → combate → objetos que cambian el mapa →
   formas de mapa). Cada mecánica trae su `BotAction` y sus datos `.tres`.
2. **Cada regla nueva entra con su test** en `tests/` (eso es lo que avisa si algo se rompe después).
3. **Tú pruebas en vivo**; el agente corre `verificar.ps1` (compila, tests, carga en Godot).
4. **Al cerrar cada etapa**, `verificar.ps1 -Estres` para comprobar que el rendimiento sigue dentro del presupuesto.
5. **Gráficos al final (Etapa 7)**: como la vista está separada de la lógica, se cambian las vistas
   sin tocar las reglas.
