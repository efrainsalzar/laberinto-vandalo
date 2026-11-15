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
	[string]$Escena = ""
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
} finally {
	Pop-Location
}

Write-Host "`n=== Resumen ===" -ForegroundColor Cyan
$resultados.GetEnumerator() | ForEach-Object {
	$color = if ($_.Value -eq 'OK') { 'Green' } else { 'Red' }
	Write-Host ("{0,-22} {1}" -f $_.Key, $_.Value) -ForegroundColor $color
}
exit ([int]$fallo)
