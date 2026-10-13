# The field's flowers (scripts/blender/arena_flowers.py): builds the flower models in art/arena/Flowers.blend the first
# time (and any kind the file does not have yet), then exports every flower_* object of its "Flowers" collection to
# media/tumbu/arena/flower_<kind>.mesh. Grass.cpp plants them (media/configuration/grass.object, "flowers field").
# Usage: .\scripts\arena-flowers.ps1 [-Rebuild] [-Blender <path to blender.exe>]
#   -Rebuild  builds every flower again from the script: DISCARDS hand edits of them in Flowers.blend
# After editing a flower by hand in Blender (shape, Vertex Paint colours), run it without -Rebuild to export.
param(
    [switch]$Rebuild,
    [string]$Blender = 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path $Blender)) { throw "Blender not found: $Blender (pass -Blender)" }

$scriptArgs = @('--background', '--python-exit-code', '1', '--python', (Join-Path $PSScriptRoot 'blender\arena_flowers.py'), '--', $root)
if ($Rebuild) { $scriptArgs += '--rebuild' }

# Blender prints to stderr; Windows PowerShell 5.1 would turn that into errors under 'Stop' (the exit code decides).
$ErrorActionPreference = 'Continue'
& $Blender @scriptArgs 2>&1 | ForEach-Object {
    $line = "$_"
    if ($line -match '^\[FLOWERS\]|Error|Traceback|^\s+File |WARN') { Write-Host $line }
}
$ErrorActionPreference = 'Stop'
if ($LASTEXITCODE -ne 0) { throw "Blender failed (exit $LASTEXITCODE)" }
