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
	[switch]$Estres
)

$ErrorActionPreference = 'Stop'
$raiz = Split-Path -Parent $PSScriptRoot
$scons = if ($env:SCONS) { $env:SCONS } else { 'scons' }
$godot = if ($env:GODOT_CONSOLE) { $env:GODOT_CONSOLE } else { 'godot' }
$resultados = [ordered]@{}
$fallo = $false

function Paso($nombre, [scriptblock]$accion) {
	Write-Host "`n=== $nombre ===" -ForegroundColor Cyan
	try {
		$ok = & $accion
		$script:resultados[$nombre] = if ($ok) { 'OK' } else { 'FALLÓ' }
		if (-not $ok) { $script:fallo = $true }
	} catch {
		Write-Host $_ -ForegroundColor Red
		$script:resultados[$nombre] = 'FALLÓ'
		$script:fallo = $true
	}
}

Push-Location $raiz
try {
	if (-not $SinCompilar) {
		Paso 'Compilar extensión' {
			$salida = & $scons target=template_debug compiledb=yes -j8 2>&1
			$errores = $salida | Select-String -Pattern ' error |error C\d+|fatal error|scons: \*\*\*'
			$errores | ForEach-Object { Write-Host $_ -ForegroundColor Red }
			($LASTEXITCODE -eq 0) -and (-not $errores)
		}
	}

	Paso 'Tests unitarios' {
		$salida = & $scons tests=yes 2>&1
		$salida | Select-String -Pattern 'test cases|assertions|Status|ERROR|FAILED' | ForEach-Object { Write-Host $_ }
		($LASTEXITCODE -eq 0) -and ($salida | Select-String 'Status: SUCCESS')
	}

	if (-not $SinGodot) {
		Paso 'Godot (sin ventana)' {
			& $godot --headless --path project --import 2>&1 | Out-Null
			$argumentos = @('--headless', '--path', 'project', '--quit-after', '10')
			if ($Escena) { $argumentos += $Escena }
			$salida = & $godot @argumentos 2>&1
			$problemas = $salida | Select-String -Pattern 'ERROR|SCRIPT ERROR|Parse Error|WARNING|push_warning'
			$salida | Select-String -Pattern '\[laberinto\]' | ForEach-Object { Write-Host $_ }
			$problemas | ForEach-Object { Write-Host $_ -ForegroundColor Yellow }
			($salida | Select-String 'Extension C\+\+ cargada') -and (-not $problemas)
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
			if ($ultima) { Write-Host "Captura: $($ultima.FullName)" }
			[bool]$ultima
		}
	}
	if ($Estres) {
		Paso 'Prueba de estrés' {
			$job = Start-Job { param($g, $p) & $g --path $p -- --stress --quit-after-stress 2>&1 } -ArgumentList $godot, (Join-Path $raiz 'project')
			if (-not (Wait-Job $job -Timeout 180)) {
				Stop-Job $job
				Write-Host 'La prueba de estrés no terminó en 180 s' -ForegroundColor Red
				return $false
			}
			Receive-Job $job | Select-String -Pattern 'ERROR|SCRIPT ERROR' | ForEach-Object { Write-Host $_ -ForegroundColor Yellow }
			$informe = Get-ChildItem (Join-Path $env:APPDATA 'Godot\app_userdata\*\stress\latest.json') -ErrorAction SilentlyContinue | Select-Object -First 1
			if (-not $informe) { Write-Host 'No se encontró latest.json' -ForegroundColor Red; return $false }
			$r = Get-Content $informe.FullName -Raw -Encoding utf8 | ConvertFrom-Json
			$f = $r.frames
			Write-Host ("{0} · promedio {1} ms ({2} FPS) · p95 {3} · p99 {4} · peor {5} ms · frames >33 ms: {6} · tirones: {7}" -f `
				$r.verdict, $f.avg_ms, $f.avg_fps, $f.p95_ms, $f.p99_ms, $f.worst_ms, $f.over_33_ms, $r.spikes.Count)
			$r.systems | Select-Object -First 4 | ForEach-Object { Write-Host ("  {0,-18} {1,8} ms/s  peor {2} ms" -f $_.name, $_.avg_ms_per_s, $_.worst_ms) }
			Write-Host "  Informe: $($informe.FullName)"
			$r.verdict -eq 'DENTRO'
		}
	}
} finally {
	Pop-Location
}

Write-Host "`n=== Resumen ===" -ForegroundColor Cyan
$resultados.GetEnumerator() | ForEach-Object {
	$color = if ($_.Value -eq 'OK') { 'Green' } else { 'Red' }
	Write-Host ("{0,-22} {1}" -f $_.Key, $_.Value) -ForegroundColor $color
}
exit ([int]$fallo)
