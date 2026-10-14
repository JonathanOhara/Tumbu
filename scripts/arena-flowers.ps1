# The field's flowers and reeds (scripts/blender/arena_flowers.py): builds the plant models in art/arena/Flowers.blend
# the first time (and any kind the file does not have yet), then exports every flower_* and reed_* object of its
# "Flowers" collection to media/tumbu/arena/<name>.mesh. Grass.cpp plants them (media/configuration/grass.object,
# "flowers field" and "reeds field").
# Usage: .\scripts\arena-flowers.ps1 [-Rebuild] [-Only flower_daisy,reed_pampas] [-Preview] [-Blender <path to blender.exe>]
#   -Rebuild  builds the plants again from the script: DISCARDS hand edits of them in Flowers.blend (with -Only: only
#             those)
#   -Only     only these plants (object names, comma-separated) are built, rebuilt, exported or previewed
#   -Preview  renders the plants (or the -Only ones) side by side into %USERPROFILE%\Tumbu\plants-preview.png, as
#             Flowers.blend has them; saves nothing and exports nothing
# After editing a plant by hand in Blender (shape, Vertex Paint colours), run it without -Rebuild to export.
param(
    [switch]$Rebuild,
    [string]$Only = '',
    [switch]$Preview,
    [string]$Blender = 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
if (-not (Test-Path $Blender)) { throw "Blender not found: $Blender (pass -Blender)" }

$scriptArgs = @('--background', '--python-exit-code', '1', '--python', (Join-Path $PSScriptRoot 'blender\arena_flowers.py'), '--', $root)
if ($Rebuild) { $scriptArgs += '--rebuild' }
if ($Only) { $scriptArgs += @('--only', $Only) }
if ($Preview) { $scriptArgs += @('--preview', (Join-Path $env:USERPROFILE 'Tumbu\plants-preview.png')) }

# Blender prints to stderr; Windows PowerShell 5.1 would turn that into errors under 'Stop' (the exit code decides).
$ErrorActionPreference = 'Continue'
& $Blender @scriptArgs 2>&1 | ForEach-Object {
    $line = "$_"
    if ($line -match '^\[FLOWERS\]|Error|Traceback|^\s+File |WARN') { Write-Host $line }
}
$ErrorActionPreference = 'Stop'
if ($LASTEXITCODE -ne 0) { throw "Blender failed (exit $LASTEXITCODE)" }
