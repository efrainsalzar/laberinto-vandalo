# 🏆 Laberinto Vándalo — Idea del juego

> Documento de visión inicial. Reúne todas las ideas del juego en un solo lugar.
> Más adelante se separará en documentos específicos (reglas, personajes, arquitectura, etc.).
>
> **Nombre provisional.** Otras opciones: *Trofeo Maldito*, *Ratas del Laberinto*, *Maze Bandits*.

---

## 1. Concepto en una frase

**Un torneo en un laberinto vivo donde varios participantes compiten por un trofeo, pero unos vándalos sabotean todo para robárselo. Solo gana uno: quien consiga el trofeo y vuelva con él a su zona de resguardo antes de que se acabe el tiempo.**

---

## 2. Historia y temática

### La premisa
Se organiza un torneo: varios participantes entran a un gran laberinto para ganar un trofeo legendario. Lo que nadie sabe es que todo fue planeado por los **vándalos**, que modificaron el laberinto y se infiltraron para quedarse con el trofeo.

Lo que debía ser un simple torneo se sale de control y se convierte en una guerra entre participantes, vándalos, soldados y aliados. Pero el objetivo de cada participante no cambia: **vino a ganar el trofeo.**

Todo lo que pasa en medio es caos que el jugador no controla: solo puede reaccionar.

### Tono
- **Serio en lo que está en juego:** hay vidas, muertes y una sola victoria.
- **Los vándalos aportan la parte provocadora:** se burlan, humillan y roban. Su función es que el jugador los odie.
- **Épico, impredecible y con momentos graciosos** que salen solos del caos (sobre todo por las personalidades de los bots).

### Estética (por definir)
Opciones propuestas:
1. **Mansión o castillo encantado:** paredes que se mueven como por magia y vándalos como bandidos enmascarados.
2. **Arcade neón ciberpunk:** el laberinto es una red digital, los vándalos son hackers o virus y los portales son glitches.
3. **Templo antiguo:** trampas clásicas, guardianes y saqueadores.

Estilo gráfico: **2D, vista desde arriba (top-down)**, probablemente pixel art.

### Regla de oro legal
Todo es **original**: personajes, nombres, arte, música y código. Las mecánicas clásicas (laberinto, recolectar, escapar de enemigos con personalidad) son libres de usar, pero **no se copia la apariencia, los nombres ni los sonidos de ninguna franquicia existente.**

---

## 3. Objetivo y condición de victoria

- **Gana un solo participante:** el que toma el trofeo y lo lleva a **su propia zona de resguardo** (donde empezó).
- Si el **tiempo se acaba** y nadie lo logró, **los vándalos se llevan el trofeo y nadie gana.**
- Eliminar vándalos **no termina el juego.** Los vándalos son obstáculos, no el objetivo: siguen apareciendo y cada vez son más agresivos.

---

## 4. Estructura de una partida (≈ 8–12 minutos)

| Fase | Qué pasa |
|---|---|
| **1. Preparación** | Explorar, juntar puntos, stats, armas y escudos. Pocos vándalos. La bóveda del trofeo está vigilada. |
| **2. Asalto** | Los participantes atacan la bóveda y pelean contra los soldados guardianes, y entre ellos. Quien toma el trofeo se vuelve el objetivo de todos. |
| **3. El golpe de los vándalos** | El mapa cambia más rápido, las zonas de resguardo pueden moverse y los vándalos atacan en masa para robar el trofeo antes de que termine el tiempo. |

---

## 5. Participantes

- **Cantidad dinámica:** de 2 a 8. Cada lugar puede ocuparlo un **humano o un bot.**
- **Al inicio del desarrollo: 1 humano + bots** (o solo bots).
- Cada participante tiene:
  - **Barra de salud** (por ejemplo, 100).
  - **3 vidas** por partida.
  - **Stats mejorables:** velocidad, resistencia, visión, ataque.
  - **Habilidades propias** según el personaje.
  - **Inventario:** armas, pociones, escudos y el trofeo.

### Muerte y eliminación
- Al perder la salud: pierdes una vida y **reapareces en tu zona de resguardo** con unos segundos de invulnerabilidad.
- Si llevabas el trofeo, **se cae** donde moriste y queda marcado para todos.
- Al perder **las 3 vidas**, quedas eliminado: **los vándalos llegan a saquearte.** Te roban accesorios y stats, se burlan y te rematan con un *finisher* que te borra del mapa. (La idea es generar odio hacia ellos.)
- **Los eliminados quedan como fantasmas espectadores:** pueden mirar la partida y **ven en todo momento dónde están los vándalos.** "Los muertos no hablan": no pueden comunicarse ni intervenir.

---

## 6. El trofeo

- Empieza dentro de una **bóveda custodiada por soldados** (funcionan como mini jefes). Cuesta entrar, pero una vez dentro es fácil sacarlo.
- Su aparición puede ser **aleatoria por probabilidad**: en el punto más lejano de los jugadores o con más probabilidad cerca de quien tiene más puntos acumulados.
- Quien lo lleva:
  - Va **más lento.**
  - Es **visible para todos.**
  - **No puede curarse** mientras lo carga (por definir).
- Los vándalos pueden robarlo y huir hacia su guarida. Si escapan, nadie gana.

---

## 7. El laberinto vivo

- **Generado por código** en cada partida: nunca es igual.
- **Cambia en tiempo real:** paredes que se mueven, caminos que se abren y cierran.
- **Pasadizos secretos** y **portales.**
- **Las zonas de resguardo (bases) pueden cambiar de posición**, así cada partida termina de forma diferente.
- **Visión limitada (niebla de guerra):** solo ves lo que tienes cerca. No ves a los demás hasta que están cerca.
- **Eventos:** apagones, zonas que se cierran, temblores.

---

## 8. Objetos

| Tipo | Ejemplos |
|---|---|
| **Puntos / comida** | Aparecen por todo el mapa. Dan stats y power-ups. |
| **Power-ups** | Más velocidad, resistencia o visión, de forma temporal o permanente. |
| **Pociones** | Pequeña (común), grande (rara) y vida extra (muy rara, en zonas peligrosas). |
| **Escudos** | Burbuja (absorbe un golpe), frontal (bloquea de frente), espejo (devuelve el ataque) y anti-robo (los vándalos no pueden quitarte nada). |
| **Armas** | Por definir. |
| **Bombas y minas** | Daño en área y trampas colocables. |
| **Trampas** | Por definir. |

---

## 9. Personajes controlados por la IA

### 😈 Vándalos (los villanos)
- Aparecen **en cualquier momento**, controlados por el director.
- Actúan como un jugador más:
  - Roban power-ups justo antes de que los consigas.
  - Atacan por la espalda y te bajan la vida.
  - Ponen minas y emboscadas.
  - Pueden **robar el trofeo.**
  - **Se burlan** de todas las maneras posibles.
  - Saquean y rematan a los eliminados.
- **Nunca se acaban:** matar a uno da un respiro, pero vienen más, y cada vez más seguido.
- **Matarlos conviene:** sueltan lo que robaron.
- **Personalidades:** noob (falla y se traba), pro (frío y coordinado), fanfarrón (juega bien pero se detiene a burlarse), cobarde (huye y vuelve con refuerzos).

### 🛡️ Soldados guardianes
- Custodian la bóveda del trofeo.
- Son la primera barrera, como mini jefes. Retrasan, pero no son imposibles.
- Pelear contra ellos te debilita, y un rival puede aprovechar para emboscarte al salir.

### ✨ Personajes virtuales (aliados)
- Aparecen para **ayudar a un participante y perjudicar a los demás.**
- Dan atajos, stats, abren portales, etc.
- Suelen ayudar más a quien va perdiendo, para equilibrar.

### 🤖 Bots participantes
- Rellenan los lugares cuando no hay suficientes jugadores humanos.
- Tienen personalidad y nivel de habilidad: a veces juegan como pros y a veces como noobs, para dar gracia y sorpresa.

---

## 10. El Director de IA (el cerebro del caos)

- Decide **cuándo y dónde** aparecen vándalos, aliados, objetos y eventos.
- **Coordina ataques en equipo:** los vándalos se reposicionan y atacan a la vez, con roles (uno distrae, otros flanquean, otro corta la retirada) y estrategias conocidas.
- **Aprende del jugador:** registra sus rutas favoritas, dónde se esconde y cómo reacciona, y prepara emboscadas ahí.
- **Ajusta la dificultad:** más presión si alguien va arrasando y menos si todos la están pasando mal.
- **Ataques justos:** los vándalos se anuncian (una risa, un silbido, una sombra) para que el jugador tenga oportunidad de reaccionar. Si pierde, que piense "me ganaron bien", no "el juego hace trampa".

---

## 11. Torneo

- Varias rondas por torneo (cantidad por definir).
- Puntos por trofeos conseguidos, eliminaciones y vándalos derrotados.
- Al final, un campeón.

---

## 12. Modos de juego (futuro)

- **Contra bots** (el primero que se desarrolla).
- **Multijugador local.**
- **En línea**, con bots rellenando los lugares vacíos.
- **Modo caos:** cada cierto tiempo cambia una regla al azar.

---

## 13. Ideas guardadas para el futuro

Ideas que salieron en la lluvia de ideas. **No van en la primera versión**, pero forman parte del universo:

- **Vándalos obligados:** algunos vándalos no son malos, los obligan a jugar. Se notaría jugando (dudan, fallan a propósito, llevan una marca o cadena). Al eliminarlos descubres quiénes eran.
- **Traición:** un vándalo puede traicionar a su bando y ayudarte. Los demás vándalos lo ejecutan frente a todos.
- **Venganza:** el vándalo que te robó vuelve en la siguiente ronda con tus cosas puestas y un nombre propio. Si lo derrotas, recuperas lo tuyo con un bonus. *(Ojo: el "Sistema Némesis" de Warner Bros está patentado; hacer una versión propia y más simple.)*
- **Compañeros animales:** conocen el camino, dan más visión y detectan trampas.
- **Prisioneros** que rescatar.
- **Vándalos jugables**, con su propia historia y personajes.
- **Modo historia** que cuente qué hay detrás del torneo.
- **Versión 3D** en el futuro (por ejemplo, con Unreal Engine).

---

## 14. Tecnología

| Área | Herramienta |
|---|---|
| Motor | **Godot 4** (2D, gratis, open source MIT) |
| Lenguaje principal | **C++** vía **GDExtension** (godot-cpp) para la lógica, la IA y el laberinto |
| Lenguaje secundario | **GDScript** para la interfaz y la conexión de escenas |
| Compilación | SCons |
| Editor | VS Code (C/C++ o clangd, godot-tools, GitLens, EditorConfig) |
| Tests | doctest |
| Calidad | clang-format, clang-tidy |
| Versiones | Git + GitHub |
| Arte | Pixelorama / LibreSprite / Krita, Kenney.nl para prototipos |
| Sonido | jsfxr / ChipTone, LMMS, Audacity |
| IA para programar | Claude Code + `CLAUDE.md` con el contexto del proyecto |

**Decisión:** se usa Godot desde el día uno, sin diseñar para "migrar de motor" (evitar la sobreingeniería). Sí se mantiene código limpio: la lógica separada de lo visual.

---

## 15. Arquitectura y patrones de diseño

**Idea central: el participante no sabe quién lo controla.**

```
Participante
 ├─ Salud / Vidas      (componente)
 ├─ Stats              (componente)
 ├─ Inventario         (componente)
 ├─ Visión             (componente)
 └─ Controlador        (intercambiable)
      ├─ ControladorHumano
      ├─ ControladorBot
      └─ ControladorRed  (futuro)
```

| Patrón | Uso |
|---|---|
| **Strategy** | Controladores (humano, bot, red) y personalidades de bots y vándalos |
| **State** | Estados del participante (vivo, reapareciendo, con trofeo, eliminado) y fases de la partida |
| **Command** | Acciones como órdenes (mover, atacar, usar objeto): base del multijugador y las repeticiones |
| **Observer / señales** | Eventos: trofeo tomado, jugador murió, vándalo apareció |
| **Composición** | Componentes en lugar de herencia profunda |
| **Factory + datos** | Objetos, vándalos y soldados definidos en archivos de datos |
| **Decorator / modificadores** | Power-ups y escudos que se apilan sobre los stats |
| **Composite** | Árboles de comportamiento para la IA |

### Módulos principales
- **Gestor de partida:** fases, reloj, victoria y cantidad de participantes.
- **Generador de laberinto:** creación y mutación en tiempo real.
- **Director de IA.**
- **Cerebro de bots y vándalos.**
- **Sistema de visión** (niebla de guerra).
- **Sistema de objetos** (basado en datos).
- **Configuración de reglas** en un archivo (tiempo, vidas, jugadores, etc.).

---

## 16. Reglas del núcleo (versión 0.1)

Lo mínimo para el primer prototipo:

1. Cada participante empieza en su zona de resguardo.
2. Reloj de 8 minutos.
3. El trofeo está en una bóveda en el centro, custodiada por 2–3 soldados.
4. Por el mapa aparecen puntos que suben velocidad, vida o visión.
5. 100 de salud y 3 vidas. Al morir se reaparece en la base. Con 0 vidas, eliminado.
6. Quien lleva el trofeo va más lento y es visible para todos.
7. Si muere con el trofeo, el trofeo cae al piso.
8. Gana quien lleva el trofeo a su base. Si se acaba el tiempo, nadie gana.
9. Visión limitada alrededor de cada participante.

---

## 17. Hoja de ruta

1. **Prototipo:** laberinto generado, un participante, visión limitada, puntos, trofeo, regreso a la base.
2. **Bots:** participantes controlados por la IA que buscan el trofeo.
3. **Combate:** salud, vidas, armas, bombas, minas, escudos y pociones.
4. **Soldados guardianes** y la bóveda.
5. **Vándalos:** robo, burlas, saqueo y el director básico.
6. **Mapa vivo:** paredes que cambian, portales, bases que se mueven.
7. **Demo pública en itch.io.**
8. **Aliados virtuales, torneo con rondas y multijugador local.**
9. **Juego en línea.**
10. **Steam.**

> **Principio:** la fase 1 tiene que ser divertida por sí sola. Todo lo demás se agrega encima, poco a poco.

---

## 18. Proyecto público

- **Código:** licencia MIT.
- **Nombre, personajes y arte:** reservados (por definir).
- Repositorio con README, CONTRIBUTING, hoja de ruta y tareas para colaboradores ("good first issue").
- Comunidad: Discord, devlogs en itch.io y videos cortos del progreso en redes.

---

## 19. Preguntas abiertas

- [ ] Nombre definitivo del juego.
- [ ] Estética final (mansión, neón, templo u otra).
- [ ] Cantidad de rondas por torneo y cómo se puntúa.
- [ ] Lista de participantes iniciales y sus habilidades.
- [ ] Tipos de armas y trampas.
- [ ] Qué se conserva entre rondas.
- [ ] Duración exacta de cada fase.
