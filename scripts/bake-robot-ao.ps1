# Bakes the robots' ambient occlusion (scripts/blender/robot_ao.py) from art/robots/robot00N.blend into
# media/tumbu/robot00N/AO<part>UV_00N.png. The .mesh files are not touched (the AO uses the texture UVs).
# The art/robots files come from scripts/convert-legacy-blend.ps1 -Robots (once).
# Usage: .\scripts\bake-robot-ao.ps1 [-Robot 1] [-Blender <path to blender.exe>]
param(
    [int[]]$Robot = @(1, 2, 3, 4, 5),
    [string]$Blender = 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path $Blender)) { throw "Blender not found: $Blender (pass -Blender)" }
foreach ($n in $Robot) {
    $blend = Join-Path $root "art\robots\robot00$n.blend"
    if (-not (Test-Path $blend)) { throw "$blend not found: run .\scripts\convert-legacy-blend.ps1 -Robots first." }
    & $Blender --background $blend --python (Join-Path $PSScriptRoot 'blender\robot_ao.py') -- $root 2>&1 |
        ForEach-Object { $line = "$_"; if ($line -match '^\[ROBOT-AO\]|Error|Traceback|^\s+File ') { Write-Host $line } }
    if ($LASTEXITCODE -ne 0) { throw "Blender failed on robot00$n (exit $LASTEXITCODE)" }
}
