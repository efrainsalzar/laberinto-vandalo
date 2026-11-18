<#
.SYNOPSIS
  Verificación completa del proyecto: compila la extensión, corre los tests y ejecuta Godot sin ventana.

.DESCRIPTION
  Pensado para usarse antes de cada commit (y por la skill /verificar de Claude Code).
  Devuelve código de salida 0 si todo está bien y 1 si algo falló.

  Herramientas: usa `scons` y `godot` del PATH, o las variables de entorno SCONS y GODOT_CONSOLE
  (las define D:\Github\gamedev\env.ps1). GODOT_CONSOLE debe ser el ejecutable *_console.exe
  en Windows para poder leer la salida.

.PARAMETER SinCompilar
  Omite la compilación de la extensión (solo tests + Godot).

.PARAMETER SinGodot
  Omite la ejecución en Godot (solo compilación + tests).

.PARAMETER Captura
  Carpeta donde guardar una captura (PNG) del juego tras unos segundos. Abre una ventana.

.PARAMETER Estres
  Ejecuta además la prueba de estrés (~40 s, abre una ventana) y resume su informe
  (user://stress/latest.json). Falla si el veredicto es FUERA del presupuesto.

.PARAMETER Detalle
  Muestra toda la salida de cada paso. Sin esta opción la salida es corta (pensada para ahorrar
  tokens cuando la lee un agente): una línea por paso, y el detalle solo de los pasos que fallan.

.PARAMETER Escena
  Escena a ejecutar (por defecto la principal del proyecto). Ej: res://scenes/sandbox/maze_sandbox.tscn

.EXAMPLE
  ./tools/verificar.ps1
  ./tools/verificar.ps1 -SinCompilar
  ./tools/verificar.ps1 -Captura $env:TEMP\capturas
#>
param(
	[switch]$SinCompilar,
	[switch]$SinGodot,
	[string]$Captura = "",
	[string]$Escena = "",
	[switch]$Estres,
	[switch]$Detalle
)

$ErrorActionPreference = 'Stop'
$raiz = Split-Path -Parent $PSScriptRoot
$scons = if ($env:SCONS) { $env:SCONS } else { 'scons' }
$godot = if ($env:GODOT_CONSOLE) { $env:GODOT_CONSOLE } else { 'godot' }
$resultados = [ordered]@{}
$fallo = $false

# Info: detalle que solo se muestra si el paso falla o con -Detalle.
# Dato: una línea corta que siempre se muestra (número de tests, veredicto, ruta de la captura).
$script:bufferPaso = [System.Collections.Generic.List[object]]::new()
function Info($texto, $color = 'Gray') { $script:bufferPaso.Add(@($texto, $color)) }
function Dato($texto) { $script:resultados[$script:pasoActual] += " · $texto" }

function Paso($nombre, [scriptblock]$accion) {
	$script:bufferPaso.Clear()
	$script:pasoActual = $nombre
	$script:resultados[$nombre] = ''
	try {
		$ok = [bool](& $accion | Select-Object -Last 1)
	} catch {
		Info "$_" 'Red'
		$ok = $false
	}
	$script:resultados[$nombre] = $(if ($ok) { 'OK' } else { 'FALLÓ' }) + $script:resultados[$nombre]
	if (-not $ok) { $script:fallo = $true }
	if ($Detalle -or -not $ok) {
		Write-Host "--- $nombre ---" -ForegroundColor Cyan
		$script:bufferPaso | ForEach-Object { Write-Host $_[0] -ForegroundColor $_[1] }
	}
}

Push-Location $raiz
try {
	if (-not $SinCompilar) {
		Paso 'Compilar extensión' {
			$salida = & $scons target=template_debug compiledb=yes -j8 2>&1
			$errores = $salida | Select-String -Pattern ' error |error C\d+|fatal error|scons: \*\*\*'
			$errores | Select-Object -First 20 | ForEach-Object { Info "$_" 'Red' }
			($LASTEXITCODE -eq 0) -and (-not $errores)
		}
	}

	Paso 'Tests unitarios' {
		$salida = & $scons tests=yes 2>&1
		$salida | Select-String -Pattern 'test cases|assertions|Status|ERROR|FAILED|ERROR:|CHECK|REQUIRE' | ForEach-Object { Info "$_" }
		$casos = $salida | Select-String -Pattern 'test cases:\s*(\d+)' | Select-Object -Last 1
		if ($casos) { Dato "$($casos.Matches[0].Groups[1].Value) tests" }
		($LASTEXITCODE -eq 0) -and [bool]($salida | Select-String 'Status: SUCCESS')
	}

	if (-not $SinGodot) {
		Paso 'Godot (sin ventana)' {
			& $godot --headless --path project --import 2>&1 | Out-Null
			$argumentos = @('--headless', '--path', 'project', '--quit-after', '10')
			if ($Escena) { $argumentos += $Escena }
			$salida = & $godot @argumentos 2>&1
			$problemas = $salida | Select-String -Pattern 'ERROR|SCRIPT ERROR|Parse Error|WARNING|push_warning'
			$salida | Select-String -Pattern '\[laberinto\]' | ForEach-Object { Info "$_" }
			$problemas | Select-Object -First 20 | ForEach-Object { Info "$_" 'Yellow' }
			[bool]($salida | Select-String 'Extension C\+\+ cargada') -and (-not $problemas)
		}
	}

	if ($Captura) {
		Paso 'Captura' {
			New-Item -ItemType Directory -Force $Captura | Out-Null
			Get-ChildItem $Captura -Filter 'captura*.png' -ErrorAction SilentlyContinue | Remove-Item
			$argumentos = @('--path', 'project', '--write-movie', (Join-Path $Captura 'captura.png'), '--fixed-fps', '10', '--quit-after', '5')
			if ($Escena) { $argumentos += $Escena }
			& $godot @argumentos 2>&1 | Out-Null
			$ultima = Get-ChildItem $Captura -Filter 'captura*.png' | Sort-Object Name | Select-Object -Last 1
			if ($ultima) { Dato $ultima.FullName }
			[bool]$ultima
		}
	}
	if ($Estres) {
		Paso 'Prueba de estrés' {
			$inicio = Get-Date
			$job = Start-Job { param($g, $p) & $g --path $p -- --stress --quit-after-stress 2>&1 } -ArgumentList $godot, (Join-Path $raiz 'project')
			if (-not (Wait-Job $job -Timeout 180)) {
				Stop-Job $job
				Info 'La prueba de estrés no terminó en 180 s' 'Red'
				return $false
			}
			Receive-Job $job | Select-String -Pattern 'ERROR|SCRIPT ERROR' | ForEach-Object { Info "$_" 'Yellow' }
			$informe = Get-ChildItem (Join-Path $env:APPDATA 'Godot\app_userdata\*\stress\latest.json') -ErrorAction SilentlyContinue | Select-Object -First 1
			if (-not $informe) { Info 'No se encontró latest.json' 'Red'; return $false }
			# Un informe viejo no vale: la prueba de esta vez no llegó a escribirlo (falló o se cerró antes).
			if ($informe.LastWriteTime -lt $inicio) {
				Info "latest.json es de $($informe.LastWriteTime): la prueba de estrés no escribió un informe nuevo" 'Red'
				return $false
			}
			$r = Get-Content $informe.FullName -Raw -Encoding utf8 | ConvertFrom-Json
			$f = $r.frames
			Dato ("{0} · promedio {1} ms ({2} FPS) · p95 {3} · p99 {4} · peor {5} ms · frames >33 ms: {6} · tirones: {7}" -f `
				$r.verdict, $f.avg_ms, $f.avg_fps, $f.p95_ms, $f.p99_ms, $f.worst_ms, $f.over_33_ms, $r.spikes.Count)
			$r.systems | Select-Object -First 4 | ForEach-Object { Info ("  {0,-18} {1,8} ms/s  peor {2} ms" -f $_.name, $_.avg_ms_per_s, $_.worst_ms) }
			Info "  Informe: $($informe.FullName)"
			$r.verdict -eq 'DENTRO'
		}
	}
} finally {
	Pop-Location
}

$resultados.GetEnumerator() | ForEach-Object {
	$color = if ($_.Value.StartsWith('OK')) { 'Green' } else { 'Red' }
	Write-Host ("{0,-22} {1}" -f $_.Key, $_.Value) -ForegroundColor $color
}
exit ([int]$fallo)
