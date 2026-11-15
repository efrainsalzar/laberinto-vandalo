---
name: verificar
description: Verifica que Laberinto Vándalo compila, pasa los tests y carga en Godot sin errores, y opcionalmente toma una captura del juego. Usar antes de cada commit, después de cambiar C++/escenas/scripts, o cuando el usuario pida "verificar", "probar que todo funciona", "correr los tests" o "ver cómo se ve".
---

# /verificar

Comprobación completa del proyecto con un solo comando. La lógica vive en `tools/verificar.ps1`
(también la puede usar una persona); esta skill dice **cuándo** usarla y **cómo reportar**.

## Cómo ejecutar

Desde la raíz del repo, con el entorno activado (define `SCONS` y `GODOT_CONSOLE`):

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -Command ". D:\Github\gamedev\env.ps1 | Out-Null; ./tools/verificar.ps1; exit $LASTEXITCODE"
```

| Opción | Uso |
|---|---|
| (ninguna) | Compila + tests + Godot sin ventana |
| `-SinCompilar` | Solo cambió lógica de `core/` o tests |
| `-SinGodot` | Solo cambió `core/` y no hace falta abrir el motor |
| `-Captura <carpeta>` | Además guarda un PNG del juego (abre una ventana unos segundos). Usar la carpeta scratchpad y luego **mirar la imagen** con Read |
| `-Escena res://...` | Ejecutar una escena concreta |
| `-Estres` | Además, la **prueba de estrés** (~40 s, abre una ventana): resume `latest.json` (veredicto, percentiles, sistemas más caros, tirones). Usarla al cerrar cada etapa o tras cambios de rendimiento. Para el detalle de un tirón, leer `spikes` en el JSON. |

Si el entorno no está en `D:\Github\gamedev` (otra máquina), basta con tener `scons` y `godot` en el PATH
o definir `SCONS` y `GODOT_CONSOLE`.

## Qué revisa

1. **Compilar extensión**: `scons target=template_debug compiledb=yes`; falla si hay errores del compilador.
2. **Tests unitarios**: `scons tests=yes` (doctest); falla si algún test falla.
3. **Godot (sin ventana)**: importa el proyecto y ejecuta 10 frames; falla si no aparece
   `Extension C++ cargada` o si hay `ERROR`, `SCRIPT ERROR`, `Parse Error` o `WARNING`.
4. **Captura** (opcional).

Código de salida: `0` todo bien, `1` algo falló. Al final imprime un resumen por paso.

## Cómo reportar al usuario

- Dar el **resumen tal cual** (OK/FALLÓ por paso) y el número de tests.
- Si algo falló: mostrar la línea de error relevante, explicar la causa en lenguaje simple y proponer el arreglo.
  **Nunca** decir que "todo funciona" si un paso falló o se omitió; si se usó `-SinGodot` o `-SinCompilar`, decirlo.
- Si se tomó captura: mirarla antes de describir lo que se ve.

## Mantenimiento

Si cambia cómo se compila o se ejecuta el proyecto, se edita **solo `tools/verificar.ps1`**.
Si se agrega un paso de verificación nuevo (p. ej. clang-tidy), va en el script y se documenta en la tabla de arriba.
